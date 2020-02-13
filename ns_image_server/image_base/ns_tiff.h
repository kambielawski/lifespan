#ifndef NS_TIFF
#define NS_TIFF
#include <stdio.h>
#include "tiffio.h"
#include "ns_libtiff_interface.h"
#include "ns_image.h"

#define NS_DEFAULT_TIFF_LINES_PER_STRIP 50


struct ns_tiff_info{
	ns_tiff_info() :stripsize(0), number_of_strips(0), rows_per_strip(0), current_strip(0), strip_pos(0), strip_length(0), bytes_read_from_current_strip(0), rows_read_from_current_strip(0) {}
	//information about strip dimentions
	ns_64_bit stripsize,
				 number_of_strips,
				 rows_per_strip;
	//information used during strip reading.
	unsigned long current_strip,
				 strip_pos,
				 strip_length;
	ns_64_bit bytes_read_from_current_strip;
	ns_64_bit rows_read_from_current_strip;
};


void ns_throw_tiff_exception(const ns_ex & ex)
#ifndef _WIN32
	__attribute__((noinline));
#else
	;
#endif

void ns_get_default_tiff_parameters(const unsigned char component_size, ns_image_properties & properties,ns_tiff_info & tiff_info,TIFF * image);

struct ns_safe_tiff_client_data{
	ns_safe_tiff_client_data(){
		data.error_storage = 0;
		data.tiff_fd.fd = 0;
		data.tiff_fd.h = 0;
		data.store_errors = false;
	}
	~ns_safe_tiff_client_data(){
		clear();
	}
	bool exception_thrown(){
		return data.error_storage != 0;
	}
	ns_ex ex(){
		return *static_cast<ns_ex *>(data.error_storage);
	}
	void clear(){
		if (data.error_storage != 0){
			delete static_cast<ns_ex *>(data.error_storage);
			data.error_storage = 0;
		}	
	}
	
	ns_tiff_client_data data;
};

void ns_initialize_libtiff();

TIFF* TIFFOpen(const char* name, ns_tiff_client_data * client_data,const char* mode);

void * ns_low_mem_allocate(tmsize_t mem);

template<class ns_component>
class ns_tiff_image_input_file: public ns_image_input_file<ns_component>{

public:

 ns_tiff_image_input_file():strip_buffer(0),low_memory_single_line_reads(false),image(0){ns_initialize_libtiff();}
	//~ns_image_input_file(){close();}
	//open and close files

	~ns_tiff_image_input_file(){close();}
	bool low_memory_single_line_reads;
	void open_file(const std::string & filename){
		try{
			//Normally, we'd just want to handle errors by throwing an exception from our
			//custom TIFFError handler.  However, in TIFFOpen(), libtiff calls this error handler BEFORE
			//cleaning up its temporary files and releasing file handles.
			//Throwing an exception in the error handler therefore produces resource leaks. 
			//So, we set the custom error handler to store errors rather than throw them.
			client_data.clear();
			client_data.data.store_errors = true;
			if (0)
			image = ns_tiff_open(filename.c_str(),&client_data.data,"rCm");
			else image = ns_tiff_open(filename.c_str(),&client_data.data,"r");
			if (client_data.exception_thrown()){
				std::cerr << "1";
				ns_throw_tiff_exception(client_data.ex());
			}
			//turn off error storage to renable exception throws in the error handler.

			if (image == NULL)
				ns_throw_tiff_exception(ns_ex("TIFFOpen returned NULL"));

			ns_get_default_tiff_parameters(sizeof(ns_component),ns_image_input_file<ns_component>::_properties,tiff_info,image);
			if (client_data.exception_thrown()){
				std::cerr << "2";
				ns_throw_tiff_exception(client_data.ex());
			}

			if (ns_image_input_file<ns_component>::_properties.width == 0 ||
				ns_image_input_file<ns_component>::_properties.height == 0 ||
			    ns_image_input_file<ns_component>::_properties.components == 0){
					ns_throw_tiff_exception(ns_ex("ns_tiff_image_input_stream::open_file()::Specified file has no pixels: (")
						<< ns_image_input_file<ns_component>::_properties.width << ","
						<< ns_image_input_file<ns_component>::_properties.height << ","
						<< ns_image_input_file<ns_component>::_properties.components << ")");
			}
			
			if (!low_memory_single_line_reads) {
				strip_buffer = (ns_component *)ns_low_mem_allocate(sizeof(ns_component)*tiff_info.stripsize);
				if (strip_buffer == 0)
					ns_throw_tiff_exception(ns_ex("ns_tiff_image_input_file::Could not allocate low-mem strip buffer of length ") << tiff_info.stripsize << " for image of size (w,h,c)= (" << ns_image_input_file<ns_component>::_properties.width << "," << ns_image_input_file<ns_component>::_properties.height << "," << ns_image_input_file<ns_component>::_properties.components << ")");
			}
			else {
				unsigned long s = TIFFScanlineSize(image);
				if (s != ns_image_input_file<ns_component>::_properties.width*ns_image_input_file<ns_component>::_properties.components * sizeof(ns_component))
					throw ns_ex("Unexpected strip size: ") << s;
				strip_buffer = (ns_component *)_TIFFmalloc(s);	
				if (strip_buffer == 0)
					ns_throw_tiff_exception(ns_ex("ns_tiff_image_input_file::Could not allocate scanline buffer of length ") << s << " for image of size (w,h,c)= (" << ns_image_input_file<ns_component>::_properties.width << "," << ns_image_input_file<ns_component>::_properties.height << "," << ns_image_input_file<ns_component>::_properties.components << ")");

			}

			if (client_data.exception_thrown()){
				std::cerr << "4";
				ns_throw_tiff_exception(client_data.ex());
			}
			opened_filename = filename;
			lines_read = 0;
		}
		catch(ns_ex & ex){
			ns_throw_tiff_exception(ns_ex("ns_tiff_image_input_stream::open_file()::") << ex.text() << "::" << " \"" << ns_file_io << filename << "\"");
		}
	}
	void open_mem(const void *){
		ns_throw_tiff_exception(ns_ex("ns_tiff_image_input_file::Opening from memory is not supported.")<< ns_file_io);
	}
	void close(){
		try{
			opened_filename.clear();
			close_file();
		}
		catch(ns_ex & ex){
			std::cerr << "ns_tiff_image_input_file::close()::Could not close file correctly: " << ex.text() << "\n";
		}
		catch(...){
			std::cerr << "ns_tiff_image_input_file::close()::Unknown error encountered while closing file.\n";
		}
	}
	 void seek_to_beginning(){
		close_file();
		open_file(opened_filename);
		return ;
	}

	//read in a single line
	bool read_line(ns_component * buffer){
		try{
			if (lines_read == ns_image_input_file<ns_component>::_properties.height)
				ns_throw_tiff_exception(ns_ex("ns_tiff::read_line()::Attempting to read too many lines from file: Requested line ") << (lines_read+1) << " from an image with height " << ns_image_input_file<ns_component>::_properties.height);
			
			ns_64_bit bytes_read;
			if (strip_buffer == 0)
				ns_throw_tiff_exception("Trying to read from a broken ns_tiff object!");

			if (!low_memory_single_line_reads) {
				//read in another strip when strip buffer is empty
				if (tiff_info.rows_read_from_current_strip == tiff_info.rows_per_strip) {
					bytes_read = TIFFReadEncodedStrip(image, tiff_info.current_strip, strip_buffer, tiff_info.stripsize);
					if (client_data.exception_thrown())
						throw client_data.ex();
					tiff_info.current_strip++;
					tiff_info.rows_read_from_current_strip = 0;

				}
				//read from the buffer
				memcpy(buffer, &(strip_buffer[tiff_info.rows_read_from_current_strip*ns_image_input_file<ns_component>::_properties.width*ns_image_input_file<ns_component>::_properties.components]),
					ns_image_input_file<ns_component>::_properties.width*ns_image_input_file<ns_component>::_properties.components * sizeof(ns_component));
				tiff_info.rows_read_from_current_strip++;
			}
			else {
				bytes_read = TIFFReadScanline(image, strip_buffer, lines_read);
				if (client_data.exception_thrown())
					throw client_data.ex();
				memcpy(buffer, strip_buffer,ns_image_input_file<ns_component>::_properties.width*ns_image_input_file<ns_component>::_properties.components * sizeof(ns_component));
			}
		
		
			lines_read++;
			return true;

		}
		catch(ns_ex & ex){
			try{
			_TIFFfree(strip_buffer);
			}
			catch(...){
				std::cerr << "An error occurred freeing the strip buffer.\n";
			}
			strip_buffer = 0;
			ns_throw_tiff_exception(ex);
		}
		catch(...){
			try{
				_TIFFfree(strip_buffer);
			}
			catch(...){
				std::cerr << "An error occurred freeing the strip buffer.\n";
			}
	
			strip_buffer = 0;
			throw;
		}
		return false;
	}

	//read a position/component from the specified buffer.
	ns_component * operator()(const unsigned long x, const  unsigned int component, ns_component * buffer)const{
		return &(buffer[ns_image_input_file<ns_component>::_properties.components*x + component]);
	}
	//read in multiple lines
	const unsigned long read_lines(ns_component ** buffer, const unsigned long n) {
		for (unsigned long i = 0; i < n; i++)
			read_line(buffer[i]);
		//std::cerr << "\nlibtiff: Read " << lines_read << " lines.\n";
		return n;
	}
private:
	ns_safe_tiff_client_data client_data;
	//instantiated for different component size specifications
	unsigned long readEncodedStrip(TIFF * image, const unsigned long current_strip, ns_component * comp_buf,const unsigned long length);

	TIFF *image;
	ns_tiff_info tiff_info;
	ns_component * strip_buffer;
	std::string opened_filename;

	unsigned long lines_read;

	void close_file(){
		
		try {
			if (image != 0) {
				TIFFClose(image);
				image = 0;
			}
			if (strip_buffer != 0) {
				_TIFFfree(strip_buffer);
				strip_buffer = 0;
			}
		}
		catch(ns_ex & ex){
			strip_buffer = 0;
			std::cerr << "ns_tiff_image_input_file::close_file wants to throw an exception, but cannot: " << ex.text();
		}
		catch(std::exception e){
			strip_buffer = 0;
			ns_ex ex(e);
			std::cerr << "ns_tiff_image_input_file::close_file wants to throw an exception, but cannot: " << ex.text();
		}
		catch(...){
			strip_buffer = 0;
			std::cerr << "ns_tiff_image_input_file::close_file wants to throw an exception, but cannot: Unknown Error!";
		}
	}
	
};

typedef enum {ns_tiff_compression_none,ns_tiff_compression_lzw,ns_tiff_compression_zip,ns_tiff_compression_jp2000} ns_tiff_compression_type;
template<class ns_component>
class ns_tiff_image_output_file;

ns_tiff_compression_type ns_get_tiff_compression_type(const ns_image_type & type);

void ns_set_default_tiff_parameters(const ns_image_properties & p, const ns_tiff_compression_type & t,const unsigned long bits_per_sample, const ns_64_bit rows_per_strip,TIFF * file);

#define NS_TIFF_WIDTH ns_image_output_file<ns_component>::_properties.width*ns_image_output_file<ns_component>::_properties.components

//A NOTABLE BUG:
//Somewhere in either ns_image.h, ns_image_storage.h, or here, writing of 16 bit tiffs is broken.
//The images seem to be garbled in such a way that only the left half of the image can be read correctly
template<class ns_component>
class ns_tiff_image_output_file : public ns_image_output_file<ns_component>{
	ns_safe_tiff_client_data client_data;
public:
	ns_tiff_image_output_file(const ns_tiff_compression_type ctype=ns_tiff_compression_lzw):
	  	output_buffer_height(0),current_output_strip(0),output_buf(0),image(0),
		  lines_received(0),rows_per_strip(NS_DEFAULT_TIFF_LINES_PER_STRIP),
		  compression_type(ctype){ns_initialize_libtiff();}

	~ns_tiff_image_output_file(){
		try{close();}
		catch(...)
		{std::cerr << "~ns_tiff_image_output_file() tried to throw an exception!";}}
	//open and close files
	void open_file(const std::string & filename, const ns_image_properties & properties){
	
		try{
			client_data.clear();
			client_data.data.store_errors = true;
			image = ns_tiff_open(filename.c_str(),&client_data.data,"wb");
			//image = TIFFOpen(filename.c_str(),&client_data, "r");
			if (client_data.exception_thrown()){
				std::cerr << "6";
				ns_throw_tiff_exception(client_data.ex());
			}

			if (image == NULL)
				ns_throw_tiff_exception(ns_ex("TIFFOpen returned NULL"));
			ns_image_output_file<ns_component>::_properties = properties;
			if (properties.height == 0)
				ns_throw_tiff_exception(ns_ex("Cannot create an image with 0 height!"));
			if (properties.width == 0)
				ns_throw_tiff_exception(ns_ex("Cannot create an image with 0 width!"));
			if (properties.components == 0)
				ns_throw_tiff_exception(ns_ex("Cannot create an image with 0 pixel components!"));
			rows_per_strip = 2*1024*1024/(sizeof(ns_component)*NS_TIFF_WIDTH);
			if (rows_per_strip == 0)
				rows_per_strip = 1;
			if (rows_per_strip > properties.height)
				rows_per_strip = properties.height;

			ns_set_default_tiff_parameters(properties,compression_type,sizeof(ns_component)*8,rows_per_strip,image);

			if (client_data.exception_thrown()){
				std::cerr << "7";
				throw client_data.ex();
			}

			output_buf = (ns_component *)_TIFFmalloc(sizeof(ns_component)*rows_per_strip*NS_TIFF_WIDTH);
			
			if (client_data.exception_thrown()){
				std::cerr << "8";
				throw client_data.ex();
			}

			output_buffer_height = 0;
			current_output_strip = 0;
		}
		catch(ns_ex & ex){
			throw ns_ex("ns_tiff_image_output_file::open_file()::") << ex.text() << "::\"" << ns_file_io << filename.c_str() << "\"";
		}

	}
	void open_mem(const void *, const ns_image_properties & properties){
		throw ns_ex("ns_tiff_output_file::Opening from memory is not supported.")<< ns_file_io;
	}

	void close(){
		try{
			if (output_buf != 0){
				if (output_buffer_height != 0){
					//std::cerr  << "Flusing out remaining " << output_buffer_height <<" lines into strip " << current_output_strip << "\n";
					ns_64_bit ret = TIFFWriteEncodedStrip(image,current_output_strip,output_buf,sizeof(ns_component)*NS_TIFF_WIDTH*output_buffer_height);
					
						if (client_data.exception_thrown())
							std::cerr << "ns_tiff_image_output_file::close_file() wants to throw an exception, but cannot: " << client_data.ex().text() << "\n";
					if (ret != sizeof(ns_component)*NS_TIFF_WIDTH*output_buffer_height){
						std::cerr << "ns_tiff_image_output_file::close_file() wants to throw an exception, but cannot: Could not write ";
						std::cerr << sizeof(ns_component)*NS_TIFF_WIDTH*output_buffer_height << " bytes to tiff file; " << ret << " were written.";
					}
				}
				TIFFClose(image);
				_TIFFfree(output_buf);
				output_buf = 0;
				output_buffer_height = 0;
			}
		}
		catch(ns_ex & ex){
			std::cerr << "ns_tiff_image_output_file::()::!Throwing in close(): " << ex.text() << "\n";
		}
		catch(...){
			std::cerr << "ns_tiff_image_output_file::close()::!Throwing an unknown error\n";
		}
	}

	//write a single line
	bool write_line(const ns_component * buffer){
		for (unsigned long i = 0; i < NS_TIFF_WIDTH; i++)
			output_buf[output_buffer_height*NS_TIFF_WIDTH + i] = buffer[i];
		//memcpy(&(output_buf[output_buffer_height*NS_TIFF_WIDTH]),buffer,sizeof(ns_component)*NS_TIFF_WIDTH);
		output_buffer_height++;
		lines_received++;
		if (output_buffer_height == rows_per_strip){
		//	cerr << "Writing out strip " << current_output_strip << "\n";
			ns_64_bit ret = TIFFWriteEncodedStrip(image,current_output_strip,output_buf,sizeof(ns_component)*NS_TIFF_WIDTH*output_buffer_height);
			if (client_data.exception_thrown()){
				std::cerr << "10";
				throw client_data.ex();
			}
			if (ret != sizeof(ns_component)*NS_TIFF_WIDTH*output_buffer_height)
				throw ns_ex("Could not write ") << sizeof(ns_component)*NS_TIFF_WIDTH*output_buffer_height << " bytes to tiff file; " << ret << " were written.";
			output_buffer_height = 0;
			current_output_strip++;
		}
		return true;
	}
	void write_lines(const ns_component ** buffer,const unsigned int n){
		for (unsigned long i = 0; i < n; i++)
			write_line(buffer[i]);
	}

	//read a position/component from the specified buffer.
	virtual ns_component * operator()(const unsigned long x, const unsigned int component, ns_component * buffer)const{
		return &(buffer[ns_image_output_file<ns_component>::_properties.components*x + component]);
	}
	private:
	friend void ns_set_default_tiff_parameters(const ns_image_properties & p, const ns_tiff_compression_type & t,const unsigned int bits_per_sample, const unsigned long rows_per_strip,TIFF * file);

	ns_64_bit rows_per_strip;
	ns_tiff_compression_type compression_type;
	TIFF *image;
	ns_component * output_buf;
	ns_64_bit output_buffer_height;
	ns_64_bit current_output_strip;
	ns_64_bit lines_received;
};


template<>
unsigned long ns_tiff_image_input_file<ns_8_bit>::readEncodedStrip(TIFF * image, const unsigned long current_strip, ns_8_bit * comp_buf,const unsigned long length);

#endif
