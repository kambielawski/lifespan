#ifndef NS_MASK_MANAGEMENT_H
#define NS_MASK_MANAGEMENT_H

#include "ns_buffered_random_access_image.h"
#include "ns_progress_reporter.h"
#include "ns_xml.h"

#include "ns_font.h"
#include "ns_process_mask_regions.h"
struct ns_mask_collage_info{
	ns_64_bit experiment_id,
		sample_id,
		region_id;
	std::string database,sample_name;
	ns_vector_2i position,
				 dimentions;

	ns_mask_collage_info() :experiment_id(0),
		sample_id(0),
		region_id(0) {}
};

class ns_mask_collage_info_manager{
public:
	std::string to_string(){
		ns_xml_simple_writer xml;
		xml.add_header();
		for (unsigned int i = 0; i < collage_info.size(); i++){
			xml.start_group("image");
			xml.add_tag("e_id",ns_to_string(collage_info[i].experiment_id));
			xml.add_tag("s_id",ns_to_string(collage_info[i].sample_id));
			if (collage_info[i].region_id != 0)
				xml.add_tag("r_id",ns_to_string(collage_info[i].region_id));
			xml.add_tag("x",collage_info[i].position.x);
			xml.add_tag("y",collage_info[i].position.y);
			xml.add_tag("w",collage_info[i].dimentions.x);
			xml.add_tag("h",collage_info[i].dimentions.y);
			xml.add_tag("db", collage_info[i].database);
			xml.end_group();
		}
		xml.add_footer();
		return xml.result();
	}
	void from_string(const std::string & s){
		ns_xml_simple_object_reader reader;
		reader.from_string(s);
		if (reader.objects.size() == 0)
			throw ns_ex("ns_mask_collage_info_manager::from_string()::Could not parse image metadata");
		collage_info.resize(reader.objects.size());
		for (unsigned int i = 0; i < collage_info.size(); i++){
			collage_info[i].experiment_id = ns_atoi64(reader.objects[i].tag("e_id").c_str());
			collage_info[i].sample_id = ns_atoi64(reader.objects[i].tag("s_id").c_str());
			collage_info[i].position.x = atol(reader.objects[i].tag("x").c_str());
			collage_info[i].position.y = atol(reader.objects[i].tag("y").c_str());
			collage_info[i].dimentions.x = atol(reader.objects[i].tag("w").c_str());
			collage_info[i].dimentions.y = atol(reader.objects[i].tag("h").c_str());
			collage_info[i].region_id = 0;
			reader.objects[i].assign_if_present("db", collage_info[i].database);
			reader.objects[i].assign_if_present("r_id", collage_info[i].region_id);
		}
	}
	std::vector<ns_mask_collage_info> collage_info;
};


class ns_bulk_experiment_mask_manager{
public:
	ns_bulk_experiment_mask_manager():mask_analyzer(4096),resize_factor(10){}

	void set_resize_factor(const unsigned int i){
		resize_factor=i;
	}

	typedef enum { ns_plate_region_mask, ns_subregion_label_mask } ns_mask_type;

	void produce_mask_file(const ns_mask_type & mask_type,const std::string &db, unsigned int experiment_id, const std::string metadata_output_filename, ns_image_stream_file_sink<ns_8_bit> & reciever, ns_sql & sql, const unsigned long mask_time=0);
	static std::string metadata_filename(const std::string & image_filename) {
		std::string ret = image_filename;
		ret += ".xmp";
		return ret;
	}
	template<class source_t>
	const std::vector<std::string> & decode_mask_file(source_t & mask_file, std::ifstream * optional_metadata_file){

		std::cout << "Processing Mask File...\n";
		if (mask_file.properties().height == 0 || mask_file.properties().width <= 2*number_of_bytes_required_for_long)
			throw ns_ex("Empty mask file!");
		collage_info_manager.collage_info.resize(0);
	
		try {
			collage_info_manager.from_string(mask_file.properties().description);
		}
		catch (ns_ex & ex) {
			if (optional_metadata_file == 0) 
				throw ns_ex("Could not load metadata from image and no external file could be found: ") << ex.text();
			std::string input;
			while (true) {
				char a = optional_metadata_file->get();
				if (optional_metadata_file->fail())
					break;
				input.push_back(a);
			}
			collage_info_manager.from_string(input);
			optional_metadata_file->close();
		}
		std::cout << collage_info_manager.collage_info.size() << " Regions found:";
		for(unsigned long i = 0; i<(unsigned long)collage_info_manager.collage_info.size(); i++){
			std::cout << "image: " << i << " sample_id(" << collage_info_manager.collage_info[i].sample_id << ")\n";
			std::cout << "Collage position = " << collage_info_manager.collage_info[i].position.x << "," << collage_info_manager.collage_info[i].position.y << " to " <<
				collage_info_manager.collage_info[i].position.x + collage_info_manager.collage_info[i].dimentions.x << "," <<  collage_info_manager.collage_info[i].position.y + collage_info_manager.collage_info[i].dimentions.y << "\n";
		}
	//	collage_info_manager.collage_info.resize(1);
		scratch_filenames.resize(collage_info_manager.collage_info.size());

		ns_progress_reporter pr(collage_info_manager.collage_info.size(),10);
		for(unsigned long i = 0; i < (unsigned long)collage_info_manager.collage_info.size(); i++){
			pr((int)collage_info_manager.collage_info.size() - i);
			ns_image_buffered_random_access_output_image<ns_8_bit> decoded_image(1024);
			ns_image_properties prop(mask_file.properties());
			prop.width = collage_info_manager.collage_info[i].dimentions.x;
			prop.height = collage_info_manager.collage_info[i].dimentions.y;
			decoded_image.prepare_to_recieve_image(prop);
			scratch_filenames[i] = std::string("mask_processing_temp_") + ns_to_string(i) + ".tif";
			ns_image_storage_reciever_handle<ns_8_bit> out(image_server.image_storage.request_local_cache_storage(scratch_filenames[i],ns_tiff,1024,false));
			decoded_image.init_flush_recipient(out.output_stream());
			for (int y = 0; y < collage_info_manager.collage_info[i].dimentions.y;){
				unsigned int lines_to_send = collage_info_manager.collage_info[i].dimentions.y - y;
				if (lines_to_send > 1024)
					lines_to_send = 1024;
				for (unsigned int _y = 0; _y < lines_to_send; _y++)
					for (int x = 0; x < collage_info_manager.collage_info[i].dimentions.x; x++)
						decoded_image[y+_y][x] = mask_file[/*number_of_header_lines + */collage_info_manager.collage_info[i].position.y + y+_y][collage_info_manager.collage_info[i].position.x + x];
				decoded_image.flush_buffer(lines_to_send,out.output_stream());
				y+=lines_to_send;
			}
			out.output_stream().finish_recieving_image();
			mask_file.seek_to_beginning();
		}
		pr(collage_info_manager.collage_info.size());
		return scratch_filenames;
	}

	void process_mask_file(ns_image_standard & mask_vis);

	bool submit_plate_region_masks_to_cluster(bool balk_on_overwrite, ns_sql & sql);

	bool submit_subregion_label_masks_to_cluster(bool balk_on_overwrite, ns_sql & sql);

	void render_mask_file(const ns_image_properties & prop, std::vector<ns_image_buffered_random_access_input_image<ns_8_bit, ns_image_storage_source<ns_8_bit> > > & images, ns_image_stream_file_sink<ns_8_bit> & reciever);
	typedef enum {label_margin_buffer = 300, chunk_max_size = 1024,image_margin=50} ns_mask_constants;
private:
	unsigned long resize_factor;
	enum {number_of_bytes_required_for_long = sizeof(unsigned long)/sizeof(ns_8_bit), number_of_bytes_required_for_header_element = sizeof(ns_mask_collage_info)/sizeof(ns_8_bit)};
	//std::vector<ns_image_standard> decoded_images;

	ns_mask_collage_info_manager collage_info_manager;
	std::vector<std::string> scratch_filenames;
	ns_image_mask_analyzer<ns_8_bit > mask_analyzer;
};

#endif
