#include "ns_image_storage_handler.h"
#include "ns_image_server.h"
#include <functional>
#include <random>

using namespace std;

bool ns_fix_image(const std::string & filename, ns_image_server_sql * sql){
  cerr << "Processing " << filename << "\n";
  ns_image_server_captured_image im;
  string fn = ns_dir::extract_filename(filename);
  int a(0);
  im.from_filename(fn,a);
  //cerr << "image cid = " << im.captured_images_id << "\n";
  //cerr << "image ciid = " << im.capture_images_image_id << "\n";;
  //cerr << "capture_time = " << im.capture_time << "\n";
  //cerr << "sample_id = " << im.sample_id << "\n";

  *sql << "SELECT id, captured_image_id FROM capture_schedule WHERE sample_id = " << im.sample_id << " AND scheduled_time = " << im.capture_time;
  ns_sql_result res;
  sql->get_rows(res);
  if (res.size() != 1)
    return false;
  ns_64_bit sched_id = ns_atoi64(res[0][0].c_str()),
  sched_cid = ns_atoi64(res[0][1].c_str());
  //cerr << "SHED ID = " << sched_id << "; sched_cid = " << sched_cid << "\n";
  *sql << "SELECT image_id, problem FROM captured_images WHERE id = " << sched_cid;
  sql->get_rows(res);
  if (res.size()!= 1)
    return false;
  //cerr << "SCHED CI image_id = " << res[0][0] << "\n";
  bool shed_ci_has_blank_ciid(res[0][0] == "0");
  *sql << "SELECT id, image_id, problem FROM captured_images WHERE sample_id = " << im.sample_id << " AND capture_time = " << im.capture_time;

  sql->get_rows(res);
  if (res.size() != 1)
    return false;
  //cerr << "im_db cid = " << res[0][0] << "; im_db ciid = " << res[0][1] << "; problem = " << res[0][2] << "\n";
  ns_64_bit im_db_cid = ns_atoi64(res[0][0].c_str());
  ns_64_bit im_db_ciid = ns_atoi64(res[0][1].c_str());
  *sql << "SELECT filename, path FROM images WHERE id = " << im_db_ciid;
  sql->get_rows(res);
  if (res.size() != 1)
    return false;
  bool im_db_has_filename_specified(res[0][0].size()>0);
  //cerr << "DB: " << res[0][0] << "\n" << res[0][1] << "\n";

  if(im_db_has_filename_specified && shed_ci_has_blank_ciid){
    cerr << "This image has an inconsistancy in its schedule record.  The correct capture image has been identified.  Fixing...\n";
    *sql << "UPDATE capture_schedule SET problem = 0,captured_image_id = " << im_db_cid << " WHERE id = " << sched_id;
    sql->send_query();

    //cerr << sql->query();
  }
  return true;
}
void ns_image_storage_handler::fix_orphaned_captured_images(ns_image_server_sql * sql) const{
  //vector<string> d;
  vector<string> directories_to_process;
  directories_to_process.push_back(volatile_storage_directory);
  while(!directories_to_process.empty()){
    ns_dir dir;
    // cerr << *directories_to_process.rbegin()<< "\n";

    const std::string cur_dir(*directories_to_process.rbegin());
    directories_to_process.pop_back();
    dir.load(cur_dir);

    if (cur_dir!= volatile_storage_directory && !dir.files.empty() ){
      bool found(false);
      cerr << "Processing files in " << cur_dir << "\n";
      for (unsigned int i = 0; i < dir.files.size(); i++){
	if (dir.file_sizes[i] == 0)
	  continue;
	if (dir.files[i].find(".tif") != dir.files[i].npos){
	  //cerr << "file size: " << dir.file_sizes[i] << "\n";
	  bool res = ns_fix_image(cur_dir + DIR_CHAR_STR + dir.files[i],sql);
	  //  if (res)
	  //return;
	}
      }
    }
    // cerr << dir.dirs.size() << "\n";
    for (unsigned int i = 0; i < dir.dirs.size(); i++){
      if (dir.dirs[i] == "." || dir.dirs[i] == "..")
	continue;
      directories_to_process.push_back(cur_dir+DIR_CHAR_STR+dir.dirs[i]);
    }

  }/*
  for (unsigned int i = 0; i < directories_to_search.size(); i++){
    cerr << "Processing files in " << directories_to_search[i] << "\n";
    }*/

}



void ns_image_handler_submit_alert(const ns_alert::ns_alert_type & alert_type, const std::string & text, const std::string & detailed_text, ns_image_server_sql * sql){
	ns_alert a(text,detailed_text,alert_type,ns_alert::get_notification_type(alert_type,image_server_const.act_as_an_image_capture_server()),ns_alert::ns_rate_limited);

	ns_acquire_lock_for_scope alert_lock(image_server.alert_handler_lock, __FILE__, __LINE__);
	if (!sql->connected_to_central_database())
		image_server.alert_handler.submit_locally_buffered_alert(a);
	else
		image_server.alert_handler.submit_alert(a, *static_cast<ns_sql *>(sql));
	alert_lock.release();
}



void ns_image_handler_submit_alert_to_central_db(const ns_alert::ns_alert_type & alert_type, const std::string & text, const std::string & detailed_text){
		ns_acquire_for_scope<ns_sql> sql(image_server.new_sql_connection(__FILE__,__LINE__));

		ns_alert a(text,detailed_text,alert_type,ns_alert::get_notification_type(alert_type,image_server_const.act_as_an_image_capture_server()),ns_alert::ns_rate_limited);

		image_server.alert_handler.submit_alert(a,sql());
		sql.release();
}

void ns_image_handler_register_server_event(const ns_image_server_event & ev,ns_image_server_sql * sql){
	image_server_const.register_server_event(ev, sql);
}
void ns_image_handler_register_server_event(const ns_ex & ev, ns_image_server_sql * sql){
	image_server_const.register_server_event(ev, sql);
}


void ns_image_handler_register_server_event_to_central_db(const ns_ex & ev){
	image_server_const.register_server_event(ns_image_server::ns_register_in_central_db, ev);
}
void ns_image_handler_register_server_event_to_central_db(const ns_image_server_event & ev){
	image_server_const.register_server_event(ns_image_server::ns_register_in_central_db, ev);
}


ns_64_bit ns_image_storage_handler_host_id(){
	return image_server_const.host_id();
}
long ns_image_storage_handler_server_timeout_interval(){
	return image_server_const.server_timeout_interval();
}

std::string ns_image_server_scratch_directory(){
	return image_server_const.scratch_directory();
}

std::string ns_image_server_miscellaneous_directory(){
	return image_server_const.miscellaneous_directory();
}

std::string ns_image_server_cache_directory(){
	return image_server_const.cache_directory();
}

void ns_full_drive_cache_panic(const ns_ex & ex,ns_image_server_sql * sql){
	image_server_const.register_server_event(ns_image_server_event("Cache cannot read from volatile storage and is pausing the current host:") << ex.text(),sql);
	image_server_const.pause_host(sql);
}

unsigned long ns_storage_request_local_cache_file_size(ns_image_storage_handler * image_storage,const std::string & filename){
	return image_storage->request_local_cache_file_size(filename);
}

bool ns_storage_delete_from_local_cache(ns_image_storage_handler * image_storage,const std::string & filename){
	return image_storage->delete_from_local_cache(filename);
}

ns_sql * ns_get_sql_connection(){
	return image_server.new_sql_connection(__FILE__,__LINE__);
}

ns_local_buffer_connection * ns_get_local_buffer_connection(){
	return image_server.new_local_buffer_connection(__FILE__,__LINE__);
}
std::string ns_get_default_partition(){
	return image_server_const.default_partition();
}



ns_image_storage_source_handle<ns_8_bit>  ns_storage_request_from_local_cache(const ns_image_storage_handler * image_storage, const std::string & filename) {
	return image_storage->request_from_local_cache(filename);
}

ns_image_storage_source_handle<ns_8_bit> ns_storage_request_from_storage(const ns_image_storage_handler * image_storage, ns_image_server_image & im, ns_sql & sql) {
	return image_storage->request_from_storage(im, &sql);
}


ns_image_storage_source_handle<ns_8_bit> ns_request_cached_image(const ns_image_storage_handler * image_storage, ns_image_server_image & im, ns_sql & sql, std::string & unique_id) {
	if (unique_id == "")
		unique_id = image_storage->add_to_local_cache(im, ns_tiff_lzw, &sql);
	return image_storage->request_from_local_cache(unique_id, false);
}
void ns_delete_from_local_cache(const ns_image_storage_handler * image_storage, const std::string & id) {
	image_storage->delete_from_local_cache(id);
}

ns_image_storage_reciever_handle<ns_8_bit> ns_storage_request_local_cache_storage(const ns_image_storage_handler * image_storage,
																				  const std::string & filename, const ns_image_type & image_type,const unsigned long max_line_length,
																				  const bool report_to_db){
	return image_storage->request_local_cache_storage(filename,image_type,max_line_length,report_to_db);
}

double ns_image_storage_handler::get_region_images_size_on_disk(const ns_64_bit region_id,const ns_processing_task t,ns_sql & sql) const{
	std::string suffix("");
	#ifdef _WIN32
	//	suffix += " /accepteula";
	#endif
	if (t == ns_process_last_task_marker){
		const ns_file_location_specification loc(this->get_base_path_for_region(region_id,&sql));
		return ns_dir::get_directory_size(loc.absolute_long_term_directory(),du_path + suffix,false);
	}
	const ns_file_location_specification loc(this->get_path_for_region(region_id,&sql,t));
	return ns_dir::get_directory_size(loc.absolute_long_term_directory(),du_path + suffix);
}
double ns_image_storage_handler::get_experiment_video_size_on_disk(const ns_64_bit experiment_id,ns_sql & sql) const{
	ns_file_location_specification spec(image_server_const.image_storage.get_path_for_experiment(experiment_id,&sql));
	return ns_dir::get_directory_size(image_server_const.image_storage.get_absolute_path_for_video(spec,false,true),du_path);
}


double ns_image_storage_handler::get_region_metadata_size_on_disk(const ns_64_bit region_id,ns_sql & sql) const{
	ns_file_location_specification loc(get_file_specification_for_movement_data(region_id,"",&sql));
	double s(ns_dir::get_directory_size(loc.absolute_long_term_directory(),du_path));

	ns_file_location_specification region_info(image_server_const.image_storage.get_base_path_for_region(region_id,&sql));
	ns_file_location_specification region_path_info(get_file_specification_for_path_data(region_info));
	s += ns_dir::get_directory_size(region_path_info.absolute_long_term_directory(),du_path);
	return s;
}

double ns_image_storage_handler::get_sample_images_size_on_disk(const ns_64_bit sample_id,const ns_processing_task t,ns_sql & sql) const{

	if (t == ns_process_thumbnail || t == ns_unprocessed){
		ns_file_location_specification loc(get_path_for_sample_captured_images(sample_id,t == ns_process_thumbnail,&sql));
		return ns_dir::get_directory_size(loc.absolute_long_term_directory(),du_path);
	}
	//look for stray files sitting at the root directory of the sample
	if (t == ns_process_last_task_marker){
		ns_file_location_specification loc(get_path_for_sample(sample_id,&sql));
		return ns_dir::get_directory_size(loc.absolute_long_term_directory(),du_path,false);
	}
	else if (t == ns_process_compile_video)
		throw ns_ex("ns_image_storage_handler::Video size calculations not yet implemented");
	else throw ns_ex("ns_image_storage_handler::get_sample_images_size_on_disk()::Cannot handle file type ") << ns_processing_task_to_string(t);

}


void ns_move_experiment_partition(ns_64_bit experiment_id, const std::string & partition, ns_sql & sql){
		std::vector<unsigned long> image_ids;
		sql << "SELECT id, mask_id FROM capture_samples WHERE experiment_id = " << experiment_id;
		ns_sql_result samples;
		sql.get_rows(samples);
		if (samples.size() == 0)
			throw ns_ex("ns_image_server::update_experiment_partition::Experiment id ") << experiment_id << " has no samples.";

		for (unsigned int i = 0; i < samples.size(); i++){
			//update captured images
			sql << "UPDATE images, captured_images SET `images`.`partition`='" << partition  << "'"
				<< " WHERE images.id = captured_images.image_id AND captured_images.sample_id = " << samples[i][0];
			sql.send_query();
			//update masks
			sql << "UPDATE images, image_masks SET `images`.`partition`='" << partition << "'"
				<< " WHERE images.id = image_masks.image_id AND image_masks.id = " << samples[i][1];
			sql.send_query();

			ns_sql_result region_info_ids;
			sql << "SELECT id FROM sample_region_image_info WHERE sample_id = " << samples[i][0];
			sql.get_rows(region_info_ids);
			for (unsigned int j = 0; j < region_info_ids.size(); j++){
				//some images are stored for the entire region
				sql << "UPDATE images, sample_region_image_info SET `images`.`partition`='" << partition << "'"
					<< " WHERE sample_region_image_info.id = " << region_info_ids[j][0]
					<< " AND (images.id = sample_region_image_info.op21_image_id || images.id = sample_region_image_info.op22_image_id )";
				sql.send_query();
				sql << "UPDATE images, sample_region_images SET `images`.`partition`='" << partition << "'"
						<< " WHERE sample_region_images.region_info_id = " << region_info_ids[j][0]
						<< " AND image_id = images.id";
				sql.send_query();
				for (unsigned int k = 1; k < (unsigned int)ns_process_last_task_marker; k++){
					sql << "UPDATE images, sample_region_images SET `images`.`partition`='" << partition << "'"
						<< " WHERE sample_region_images.region_info_id = " << region_info_ids[j][0]
						<< " AND op" << k << "_image_id = images.id";
					sql.send_query();
				}
			}

		}
		sql << "UPDATE experiments SET `partition`='" << partition << "' WHERE id=" << experiment_id;
		sql.send_query();
}


ns_image_type ns_get_image_type(const std::string & filename){
	std::string extension = ns_tolower(ns_dir::extract_extension(filename));
	if (extension == "jpg" || extension == "jpeg")
		return ns_jpeg;
	if (extension == "tif" || extension == "tiff")
		return ns_tiff_lzw;
	if (extension == "jp2" || extension == "jpk")
		return ns_jp2k;
	throw ns_ex("ns_image_storage_handler_to_disk: Could not deduce image format from filename: '") << filename << "'";

}



void ns_image_storage_handler::update_volatile_storage_directory_for_parallel_processes(unsigned long system_parallel_process_id) {
	volatile_storage_directory = volatile_storage_directory + DIR_CHAR_STR + "process_" + ns_to_string(system_parallel_process_id);
	ns_dir::create_directory_recursive(volatile_storage_directory);

}
void ns_image_storage_handler::set_directories(const std::string & _volatile_storage_directory, const std::string & _long_term_storage_directory){
	volatile_storage_directory = _volatile_storage_directory;
	long_term_storage_directory = _long_term_storage_directory;
	if (volatile_storage_directory.size() == 0)
		throw ns_ex("ns_image_storage_handler::Volatile storage must exist!") << ns_file_io;
}

ns_image_storage_reciever_handle<ns_image_storage_handler::ns_component> ns_image_storage_handler::request_storage(ns_image_server_captured_image_region & captured_image_region, const ns_image_type & image_type, const float compression_ratio, const unsigned long max_line_length, ns_image_server_sql * sql, const ns_volatile_storage_behavior volatile_storage_behavior) const{
	ns_image_server_image image;
	image.filename = captured_image_region.filename(sql);
	image.path = captured_image_region.directory(sql);
	image.partition = get_partition_for_experiment(captured_image_region.experiment_id,sql);
	bool had_to_use_volatile_storage;
	return request_storage(image, image_type, compression_ratio,max_line_length, sql,had_to_use_volatile_storage,false,volatile_storage_behavior);
}

ns_image_storage_reciever_handle<ns_image_storage_handler::ns_component> ns_image_storage_handler::request_storage_ci(ns_image_server_captured_image & captured_image, const ns_image_type & image_type, const float compression_ratio, const unsigned long max_line_length, ns_image_server_sql * sql, ns_image_server_image & output_image, bool & had_to_use_local_storage, const ns_volatile_storage_behavior volatile_storage_behavior) const{
	output_image = captured_image.make_large_image_storage(sql);
	return request_storage(output_image, image_type, compression_ratio, max_line_length, sql,had_to_use_local_storage,false,volatile_storage_behavior);
}

bool ns_image_storage_handler::long_term_storage_was_recently_writeable(const unsigned long time_cutoff_in_seconds) const{
	if (time_cutoff_in_seconds == 0 || last_check_showed_write_access_to_long_term_storage)
		return last_check_showed_write_access_to_long_term_storage;
	else
		return (ns_current_time() - time_of_last_successful_write_check) < time_cutoff_in_seconds;
}
bool ns_image_storage_handler::test_connection_to_long_term_storage(const bool test_for_write_access){
	if (long_term_storage_directory.size() == 0){
		last_check_showed_write_access_to_long_term_storage = false;
		return false;
	}
	if (test_for_write_access){
		string fname(image_server_const.host_name_out() + "_write_test");
		//clear out invalid characters
		for (unsigned int i = 0; i < fname.size(); i++){
			if (!( (fname[i] >= 'a' && fname[i] <= 'z') ||
				(fname[i] >= 'A' && fname[i] <= 'Z') ||
				(fname[i] >= '1' && fname[i] <= '9')
				|| fname[i] == '0' || fname[i] == '-' || fname[i] == '_'))
				fname[i] = '_';
		}
		ns_acquire_lock_for_scope lock(request_storage_lock,__FILE__,__LINE__);
		bool writeable(ns_dir::file_is_writeable(long_term_storage_directory + DIR_CHAR_STR + fname));
		lock.release();
		if (simulate_long_term_storage_errors) writeable = false;
		last_check_showed_write_access_to_long_term_storage = writeable;
		if (writeable)
			time_of_last_successful_write_check = ns_current_time();
		return writeable;
	}
	if (simulate_long_term_storage_errors) return false;
	return ns_dir::file_exists(long_term_storage_directory);
}

bool ns_image_storage_handler::image_exists(ns_image_server_image & image, ns_image_server_sql * sql, bool only_long_term_storage) const{
	if (image.id == 0)
		throw ns_ex("ns_image_storage_handler::No id specified when querying image existance.");
	if (image.filename.size() == 0 || image.path.size() == 0) image.load_from_db(image.id,sql);

	std::string prt;
	if (image.partition.size() != 0)
		prt = DIR_CHAR_STR + image.partition;
	std::string fname = DIR_CHAR_STR + image.path + DIR_CHAR_STR + image.filename;
	std::string lt = long_term_storage_directory + prt + fname,
		   v = volatile_storage_directory + fname;
	ns_dir::convert_slashes(lt);
	ns_dir::convert_slashes(v);
	return (ns_dir::file_exists(lt) && !simulate_long_term_storage_errors) || (!only_long_term_storage && ns_dir::file_exists(v));
}

bool ns_image_storage_handler::assign_unique_filename(ns_image_server_image & image, ns_image_server_sql * sql) const{
	if (image.id == 0)
		throw ns_ex("ns_image_storage_handler::No id specified when requesting unique filename");
	if (image.filename.size() == 0 || image.path.size() == 0) image.load_from_db(image.id,sql);
	std::string volatile_path = volatile_storage_directory + DIR_CHAR_STR + image.path;
	std::string lt_path = long_term_storage_directory + DIR_CHAR_STR;
	if (image.partition.size() != 0)
		lt_path+= image.partition + DIR_CHAR_STR;
	lt_path += image.path;

	std::string filename = image.path + DIR_CHAR_STR + image.filename;
	ns_dir::convert_slashes(volatile_path);
	ns_dir::convert_slashes(lt_path);
	ns_dir::convert_slashes(filename);

	ns_dir::create_directory_recursive(volatile_path);
	ns_dir::create_directory_recursive(lt_path);

	std::string temp_filename =volatile_path + DIR_CHAR_STR + image.filename + "TEMPORARY";
	ns_dir::convert_slashes(temp_filename);

	if (!ns_dir::file_is_writeable(temp_filename))  //check to see if the volatile directory exists
		throw ns_ex("ns_image_storage_handler::Volatile storage is not writeable for assign_unique_filename request:") << temp_filename << ns_file_io;

	//the file doesn't already exist, so nothing needs be done.
	if (!ns_dir::file_exists(long_term_storage_directory + DIR_CHAR_STR + filename) &&
		!ns_dir::file_exists(volatile_storage_directory  + DIR_CHAR_STR + filename))
		return false;

	std::string base_name = ns_dir::extract_filename_without_extension(filename),
	       extension = ns_dir::extract_extension(filename);
	unsigned long i = 1;
	std::string new_filename[2],
		   suffix;
	const unsigned int max_count = 1000;
	while(true && i < max_count){
		suffix = ns_to_string(i);
		new_filename[0] = long_term_storage_directory + DIR_CHAR_STR + base_name + "=" + suffix + "." + extension;
		new_filename[1] = volatile_storage_directory  + DIR_CHAR_STR + base_name + "=" + suffix + "." + extension;
		if (!ns_dir::file_exists(new_filename[0]) && !ns_dir::file_exists(new_filename[1]))
			break;
		//else cout << new_filename[0] << " " << new_filename[1] << "\n";
		i++;
	}
	if (i == max_count)
		throw ns_ex("ns_image_storage_handler::assign_unique_filename()::Could not write to file") << new_filename[1] << ns_file_io;

	image.filename = ns_dir::extract_filename(new_filename[0]);
//	cerr << "filename = " << image.filename << "\n";
	return true;
}


ns_istream * ns_image_storage_handler::request_metadata_from_disk(ns_image_server_image & image,const bool binary,ns_image_server_sql * sql, bool allow_db_lookup) const{
	if (image.filename.size() == 0 || image.path.size() == 0 || image.partition.size() == 0) 
		if (!allow_db_lookup || !image.load_from_db(image.id, sql))
			throw ns_ex("Could not find image record for time path image analyzer metadata");

	ns_file_location_specification spec(look_up_image_location_no_extension_alteration (image, sql));

	//try to store image in long-term storage
	if (simulate_long_term_storage_errors || long_term_storage_directory.size() == 0 || !ns_dir::file_exists(spec.long_term_directory)){
		ns_image_handler_submit_alert(ns_alert::ns_long_term_storage_error,
			"Could not access long term storage.",
			std::string("ns_image_storage_handler::request_metadata_from_disk()::Could not access long term storage while attempting to read") + spec.relative_directory + DIR_CHAR_STR + spec.filename,
			sql);
		throw ns_ex("ns_image_storage_handler::request_metadata_from_disk()::Could not access long term storage at ") << spec.long_term_directory << ns_network_io;
	}

	ns_istream * i(0);
	ns_image_type type = ns_image_type_from_filename(spec.filename);
	bool file_does_not_exist(false);
	try {
		if (type != ns_csv_gz && type != ns_wrm_gz && type != ns_xml_gz) {
			if (binary) i = new ns_istream(new std::ifstream(spec.absolute_long_term_filename().c_str(), ios_base::binary));
			else i = new ns_istream(new std::ifstream(spec.absolute_long_term_filename().c_str()));
		}
		else {
			try {	//gzstream does not fail properly on non-existant files.
				if (!ns_dir::file_exists(spec.absolute_long_term_filename().c_str()))
					file_does_not_exist = true;
				//if (binary) i = new ns_istream(new igzstream(spec.absolute_long_term_filename().c_str()));	//always load binary!
				else i = new ns_istream(new igzstream(spec.absolute_long_term_filename().c_str(), ios_base::binary | ios_base::in));
			}
			catch (const std::exception & ex) {
				throw ns_ex(ex.what());
			}
		}
	}
	catch (...) {
		if (i != 0) delete i;
		throw;
	}

	if (file_does_not_exist || (*i)().fail()){
		delete i;
		throw ns_ex("ns_image_storage_handler::request_metadata_from_disk()::Could not open file ") << spec.absolute_long_term_filename() << ns_file_io;
	}
	return i;
}

ns_ostream * ns_image_storage_handler::request_metadata_output(ns_image_server_image & image, const ns_image_type & image_type, const bool binary, ns_image_server_sql * sql) const {

	if (image.filename.size() == 0 || image.path.size() == 0 || image.partition.size() == 0) image.load_from_db(image.id, sql);
	//std::string existing_filename_extension(ns_dir::extract_extension(image.filename));


	//remove the current extension and make use the specified one
	ns_file_location_specification spec(look_up_image_location(image,sql, image_type,true));

	ns_probe_for_illegal_character(spec.absolute_long_term_filename());
	//try to store image in long-term storage
	if (!simulate_long_term_storage_errors && long_term_storage_directory.size() != 0 && ns_dir::file_exists(spec.long_term_directory)){

		ns_dir::create_directory_recursive(spec.absolute_long_term_directory());
		if (!ns_dir::file_is_writeable(spec.absolute_long_term_filename())){
			ns_image_handler_submit_alert(ns_alert::ns_long_term_storage_error,
				"Could not access long term storage.",
				std::string("ns_image_storage_handler::request_metadata_output(1)::Could not access long term storage when attempting to write ") + spec.relative_directory + DIR_CHAR_STR + spec.filename, sql);
			throw ns_ex("ns_image_storage_handler:: request_metadata_output()::Long term storage location is not writeable: ") << spec.absolute_long_term_filename() << ns_network_io;
		}
		ns_ostream * o;
		bool file_is_not_writeable = false;
		if (image_type != ns_wrm_gz && image_type != ns_csv_gz && image_type != ns_xml_gz) {
			if (binary) o = new ns_ostream(new std::ofstream(spec.absolute_long_term_filename().c_str(), ios_base::binary));
			else o = new ns_ostream(new std::ofstream(spec.absolute_long_term_filename().c_str()));
		}
		else {
			try {
				if (!ns_dir::file_is_writeable(spec.absolute_long_term_filename().c_str()))
					file_is_not_writeable = true;
				if (binary) o = new ns_ostream(new ogzstream(spec.absolute_long_term_filename().c_str(), std::ios::out | ios_base::binary));
				else 
					throw ns_ex("ns_image_storage_handler:: request_metadata_output()::gzip files must be opened as binary!");
				//o = new ns_ostream(new ogzstream(spec.absolute_long_term_filename().c_str()));
			}
			catch (const std::exception & ex) {
				throw ns_ex(ex.what());
			}
		}
		if (file_is_not_writeable || (*o)().fail()){
			delete o;
			ns_image_handler_submit_alert(ns_alert::ns_long_term_storage_error,
				"Could not access long term storage.",
				std::string("ns_image_storage_handler::request_metadata_output(2)::Could not access long term storage when attempting to write ") + spec.relative_directory + DIR_CHAR_STR + spec.filename, sql);
			throw ns_ex("ns_image_storage_handler::request_metadata_output()::Long term storage location is not writeable: ") << spec.absolute_long_term_filename() << ns_network_io;
		}
		return o;
	}

	ns_image_handler_submit_alert(ns_alert::ns_long_term_storage_error,
				"Could not access long term storage.",
				std::string("ns_image_storage_handler::request_metadata_output(3)::Could not access long term storage when attempting to write ") + spec.relative_directory + DIR_CHAR_STR + spec.filename, sql);

	throw ns_ex("ns_image_storage_handler::request_metadata_output()::Long term storage location is not writeable.") << ns_network_io;
}



ns_image_storage_reciever_handle<ns_image_storage_handler::ns_component> ns_image_storage_handler::request_storage(ns_image_server_image & image, const ns_image_type & image_type, const float compression_ratio, const unsigned long max_line_length, ns_image_server_sql * sql, bool & had_to_use_local_storage, const bool report_to_db, const ns_volatile_storage_behavior volatile_storage_behavior) const{
	std::string fname = get_storage_to_open(image, image_type, max_line_length, sql, had_to_use_local_storage, report_to_db, volatile_storage_behavior);
	if (fname.find(".tif.tif") != fname.npos) {
		throw ns_ex("Trying to write an image with the suffix .tif.tif!");
	}
	return ns_image_storage_reciever_handle<ns_image_storage_handler::ns_component>(new ns_image_storage_reciever_to_disk<ns_image_storage_handler::ns_component>(max_line_length, fname, image_type, compression_ratio, had_to_use_local_storage, output_file_permissions));
}
ns_image_storage_reciever_handle<ns_16_bit> ns_image_storage_handler::request_storage_16_bit(ns_image_server_image & image, const ns_image_type & image_type, const float compression_ratio, const unsigned long max_line_length, ns_image_server_sql * sql, bool & had_to_use_local_storage, const bool report_to_db, const ns_volatile_storage_behavior volatile_storage_behavior) const{
	std::string fname = get_storage_to_open(image, image_type, max_line_length, sql, had_to_use_local_storage, report_to_db, volatile_storage_behavior);
	return ns_image_storage_reciever_handle<ns_16_bit>(new ns_image_storage_reciever_to_disk<ns_16_bit>(max_line_length, fname, image_type,compression_ratio, had_to_use_local_storage, output_file_permissions));
}
ns_image_storage_reciever_handle<float> ns_image_storage_handler::request_storage_float(ns_image_server_image & image, const ns_image_type & image_type, const float compression_ratio, const unsigned long max_line_length, ns_image_server_sql * sql, bool & had_to_use_local_storage, const bool report_to_db, const ns_volatile_storage_behavior volatile_storage_behavior) const{
	std::string fname = get_storage_to_open(image, image_type, max_line_length, sql, had_to_use_local_storage, report_to_db, volatile_storage_behavior);
	return ns_image_storage_reciever_handle<float>(new ns_image_storage_reciever_to_disk<float>(max_line_length, fname, image_type, compression_ratio, had_to_use_local_storage, output_file_permissions));
}

std::string ns_image_storage_handler::get_storage_to_open(ns_image_server_image & image, const ns_image_type & image_type, const unsigned long max_line_length, ns_image_server_sql * sql, bool & had_to_use_local_storage, const bool report_to_db, const ns_volatile_storage_behavior volatile_storage_behavior) const{

	ns_ex stored_error[3];
	ns_file_location_specification file_location(look_up_image_location(image,sql,image_type));

	//try to store image in long-term storage
	if (volatile_storage_behavior != ns_require_volatile && !simulate_long_term_storage_errors && long_term_storage_directory.size() != 0 && long_term_storage_is_accessible(file_location,__FILE__,__LINE__)){
		try{
			//make sure location is writeable
			ns_dir::create_directory_recursive(file_location.absolute_long_term_directory());
			if (!ns_dir::file_is_writeable(file_location.absolute_long_term_filename())){
				ns_image_handler_submit_alert(ns_alert::ns_long_term_storage_error,"Could not access long term storage.",
					std::string("ns_image_storage_handler::request_storag::Could not access long term storage when attempting to write ") + file_location.relative_directory + DIR_CHAR_STR + file_location.filename,
					sql);
				throw ns_ex("ns_image_storage_handler:: request_storage(ns_image_server_image)::Long term storage location is not writeable: ") << file_location.absolute_long_term_filename() << ns_network_io;
			}

			if (verbosity >= ns_verbose){
				ns_image_server_event ev("ns_image_storage_handler::Opening LT ");
				ev << file_location.absolute_long_term_filename()<< " for output." << ns_ts_minor_event;
				ev.log = report_to_db;
				ns_image_handler_register_server_event(ev,sql);
			}
			had_to_use_local_storage = false;
			return file_location.absolute_long_term_filename();
		}
		catch (ns_ex & ex){
			ns_image_handler_register_server_event(ex,sql);
			stored_error[0] = ex;
		}
	}

	//if all else fails, store the data in volatile storage
	if (volatile_storage_behavior == ns_forbid_volatile) {
		had_to_use_local_storage = true;
		throw ns_ex("ns_image_storage_handler::request_storage()::Could not access long term storage and volatile storage was forbidden") << ns_network_io;
	}
	if (volatile_storage_directory.size() != 0 && ns_dir::file_exists(file_location.volatile_directory)){

		try{
			//make sure location is writeable
			ns_dir::create_directory_recursive(file_location.absolute_volatile_directory());
			if (!ns_dir::file_is_writeable(file_location.absolute_volatile_filename())){
				ns_image_handler_submit_alert(ns_alert::ns_volatile_storage_error,
					"Could not access volatile storage.",
					std::string("ns_image_storage_handler::request_storage(1)::Could not access volatile storage when attempting to write ") + file_location.relative_directory + DIR_CHAR_STR + file_location.filename,
					sql);

				throw ns_ex("ns_image_storage_handler::request_storage(ns_image_server_image)::Volatile storage location is not writeable: ") << file_location.absolute_volatile_filename() << ns_file_io;
			}
			if (verbosity > ns_standard){
				ns_image_server_event ev("ns_image_storage_handler::Forced to open VT ");
				ev << file_location.absolute_volatile_filename() << " for output.";
				ev.log = report_to_db;
				ns_image_handler_register_server_event(ev,sql);
			}
			had_to_use_local_storage = true;
			return file_location.absolute_volatile_filename();
		}
		catch (ns_ex & ex){
			stored_error[2] = ex;
		}
	}

	ns_image_handler_submit_alert(ns_alert::ns_volatile_storage_error,
				"Could not access volatile storage.",
				std::string("ns_image_storage_handler::request_storage(2)::Could not access volatile storage when attempting to write ") + file_location.relative_directory + DIR_CHAR_STR + file_location.filename,
				sql);

	//take note of any problems that occured accessing network long term storage
	for (unsigned int i = 0; i < 2; i++)
		if (stored_error[i].text().size() != 0)
			ns_image_handler_register_server_event(stored_error[i],sql);

	//we couldn't store the file anywhere!  Throw the volatile storage -inaccessible error.
	if (stored_error[2].text() != "")
		throw stored_error[2];
	else if (stored_error[1].text() != "")
		throw stored_error[1];
	else throw stored_error[0];
	}

ns_image_server_image ns_image_storage_handler::create_image_db_record_for_captured_image(ns_image_server_captured_image & image,ns_image_server_sql * sql, const ns_image_type & image_type) const{

	std::string dir(image.directory(sql,ns_unprocessed,true));

	string filename(image.filename_no_load_from_db(sql));
	std::string extension = ns_dir::extract_extension(filename);

	if (extension.size() == 0) {
		image_server.register_server_event(ns_image_server_event("Capture image didn't have an extension when one was expected:  ") << filename << " ( " << extension << " )",sql);
		ns_add_image_suffix(filename, image_type);
	}

	std::string full_filename = dir + DIR_CHAR_STR + filename;
	ns_dir::convert_slashes(full_filename);
	ns_dir::convert_slashes(dir);

	ns_dir::convert_to_unix_slashes(dir);

	ns_image_server_image im;
	im.filename = filename;
	im.path = dir;
	im.id = 0;

	if (image.capture_images_image_id != 0)
		cerr << "ns_image_storage_handler::create_image_db_record_for_captured_image()::Warning: Overwriting an existing capture_images_image_id\n";

	*sql << "INSERT INTO " << sql->table_prefix() << "images SET filename = '" << filename << "', path='" << dir << "',`partition`='', currently_under_processing=1";
	image.capture_images_image_id = sql->send_query_get_id();
	im.id = image.capture_images_image_id;
	return im;

}




//this function creates a file in the volatile storage (or long term storage) and creates an image db record for it and saves it
//to the image.capture_images_image_id field
ofstream * ns_image_storage_handler::request_binary_output_for_captured_image(const ns_image_server_captured_image & captured_image, const ns_image_server_image & image, bool volatile_storage,ns_image_server_sql * sql)const{

		string dir,partition;

		if (volatile_storage) dir += volatile_storage_directory;
		else{
			dir = long_term_storage_directory;
			if (captured_image.experiment_id || image.id == 0 || image.filename.size() == 0 || image.path == ""){
				throw ns_ex("ns_image_storage_handler::request_binary_output_for_captured_image()::Incomplete information provided about captured image");
			}
			partition = get_partition_for_experiment(captured_image.experiment_id,sql,true);
			if (partition.size() != 0)
				dir += DIR_CHAR_STR + partition;
		}
		dir += DIR_CHAR_STR + image.path;
		std::string filename = image.filename;
		std::string extension = ns_dir::extract_extension(filename);

		if (extension.size() == 0)
			throw ns_ex("ns_image_storage_handler::request_binary_output_for_captured_image()::No extension specified for image");

		std::string full_filename = dir + DIR_CHAR_STR + filename;
		ns_dir::convert_slashes(full_filename);
		ns_dir::convert_slashes(dir);

		ns_dir::create_directory_recursive(dir);

		if (!ns_dir::file_exists(volatile_storage_directory))
			ns_image_handler_submit_alert(ns_alert::ns_volatile_storage_error,
					"Could not access volatile storage.",
					std::string("ns_image_storage_handler::request_binary_output::Could not access volatile storage when attempting to write ") + full_filename,
					sql);

		else if ( !volatile_storage && (long_term_storage_directory.empty() || simulate_long_term_storage_errors ||  !ns_dir::file_exists(long_term_storage_directory)))
			ns_image_handler_submit_alert(ns_alert::ns_volatile_storage_error,
					"Could not access volatile storage.",
					std::string("ns_image_storage_handler::request_binary_output::Could not access volatile storage when attempting to write ") + full_filename,
					sql);

		ofstream * output = new ofstream(full_filename.c_str(), ios_base::binary);
		//if long-term storage was requested but cannot be accessed, try volatile storage
		if (output->fail())
			throw ns_ex("ns_image_storage_handler::request_binary_output()::Could not open requested position in volatile storage: ") << full_filename << ns_file_io;


	  ns_image_handler_register_server_event(ns_image_server_event("ns_image_storage_handler::Opening ",false) << full_filename << " for binary output." << ns_ts_minor_event,sql);
	  return output;
}

ofstream * ns_image_storage_handler::request_miscellaneous_storage(const std::string & filename){
  for (unsigned int i = 0; i < 2; i++){
    
    std::string dir;
    
    if (i == 1) dir = volatile_storage_directory;
    else{
      dir = long_term_storage_directory;
      std::string partition = ns_get_default_partition();
      if (partition.size() != 0) dir += DIR_CHAR_STR + partition;
    }
    dir +=DIR_CHAR_STR + ns_image_server_miscellaneous_directory();
    
    std::string fname = dir + DIR_CHAR_STR + filename;
    ns_dir::convert_slashes(fname);
    ns_dir::convert_slashes(dir);
    ns_dir::create_directory_recursive(dir);
    
    ofstream * output = new ofstream(fname.c_str(), ios_base::binary);
    try{
      if (simulate_long_term_storage_errors || output->fail()){
	ns_image_handler_submit_alert_to_central_db(
						    ns_alert::ns_long_term_storage_error,
						    "Could not access long term storage",
						    std::string("ns_image_storage_handler::request_miscellaneous_storage::Could not access volatile storage when attempting to write ") + filename);
	throw ns_ex("ns_image_storage_handler::request_miscellaneous_storage()::Could not open requested position in long term storage: ") << fname << ns_network_io;
      }
      if (verbosity > ns_standard)
	ns_image_handler_register_server_event_to_central_db(ns_image_server_event("ns_image_storage_handler::Opening ",false) << fname << " for binary output." << ns_ts_minor_event);
    }
    catch(ns_ex & ex){
      delete output;
      if (i == 0){
	cout << ex.text();
	continue;
      }else
	throw;
      
    }
    catch(...){
      delete output;
      throw;
    }
    return output;
  }
  throw ns_ex("Code path error!");
}

ofstream * ns_image_storage_handler::request_volatile_binary_output(const std::string & filename){
	std::string dir = volatile_storage_directory + DIR_CHAR_STR + ns_image_server_scratch_directory();

	std::string fname = dir + DIR_CHAR_STR + filename;
	ns_dir::convert_slashes(fname);
	ns_dir::convert_slashes(dir);
	ns_dir::create_directory_recursive(dir);
	if (!ns_dir::file_is_writeable(fname.c_str()))
		ns_image_handler_submit_alert_to_central_db(ns_alert::ns_volatile_storage_error,
		"Could not access volatile storage",
		std::string("ns_image_storage_handler::request_volatile_binary_output(1)::Could not access volatile storage when attempting to write ") + filename);

	ofstream * output = new ofstream(fname.c_str(), ios_base::binary);
	try{
		if (output->fail()){
		ns_image_handler_submit_alert_to_central_db(ns_alert::ns_volatile_storage_error,
			"Could not access volatile storage",
			std::string("ns_image_storage_handler::request_volatile_binary_output(2)::Could not access volatile storage when attempting to write ") + filename);

			throw ns_ex("ns_image_storage_handler::request_volatile_binary_output()::Could not open requested position in volatile storage: ") << fname << ns_file_io;
		}
		if (verbosity > ns_standard)
			ns_image_handler_register_server_event_to_central_db(ns_image_server_event("ns_image_storage_handler::Opening ",false) << fname << " for binary output." << ns_ts_minor_event);
	}
	catch(...){
		delete output;
		throw;
	}
	return output;
}

ifstream * ns_image_storage_handler::request_from_volatile_storage_raw(const std::string & filename) const{
	std::string dir = volatile_storage_directory + DIR_CHAR_STR + ns_image_server_scratch_directory();
	std::string input_filename = dir + DIR_CHAR_STR + filename;
	ns_dir::convert_slashes(input_filename);

	if (ns_dir::file_exists(input_filename)){
		if (verbosity > ns_standard)
			ns_image_handler_register_server_event_to_central_db(ns_image_server_event("ns_image_storage_handler::Opening raw VT ") << input_filename << " for input." << ns_ts_minor_event);
		ifstream * in(new ifstream(input_filename.c_str(), ios::in | ios::binary));
		if (in->fail()){
			delete in;

			ns_image_handler_submit_alert_to_central_db(ns_alert::ns_volatile_storage_error,
			"Could not access volatile storage",
			std::string("ns_image_storage_handler::request_from_volatile_storage_raw::Could not access volatile storage when attempting to read ") + filename);

			throw ns_ex("ns_image_storage_handler::request_volatile_storage_raw()::Could not load file from scratch: ") << input_filename << ns_file_io;
		}
		return in;

	}
	throw ns_ex("ns_image_storage_handler::request_volatile_storage_raw()::Could not load file from scratch: ") << input_filename << ns_file_io;
}

ns_image_storage_reciever_handle<ns_image_storage_handler::ns_component> ns_image_storage_handler::request_local_cache_storage(const std::string & filename,const ns_image_type & image_type, const unsigned long max_line_length, const bool report_to_db) const{

	std::string output_dir = volatile_storage_directory + DIR_CHAR_STR + ns_image_server_cache_directory(),
		   output_filename = output_dir + DIR_CHAR_STR + filename;
	ns_dir::convert_slashes(output_filename);
	ns_dir::convert_slashes(output_dir);
	//make sure location is writeable
	ns_dir::create_directory_recursive(output_dir);
	if (!ns_dir::file_is_writeable(output_filename)){
			ns_image_handler_submit_alert_to_central_db(ns_alert::ns_volatile_storage_error,
			"Could not access volatile storage",
			std::string("ns_image_storage_handler::request_local_cache_storage::Could not access volatile storage when attempting to write ") + filename);

		throw ns_ex("ns_image_storage_handler::request_local_cache_storage()::Volitile storage location is not writeable: ") << output_filename << ns_file_io;
	}
	if (verbosity > ns_standard && report_to_db){
		ns_image_server_event ev("ns_image_storage_handler::Opening ");
		ev << output_filename << " for output.";
		ev.log = report_to_db;
		ns_image_handler_register_server_event_to_central_db(ev);
	}
	return ns_image_storage_reciever_handle<ns_image_storage_handler::ns_component>(new ns_image_storage_reciever_to_disk<ns_image_storage_handler::ns_component>(max_line_length, output_filename, image_type,true, output_file_permissions));
}

ns_image_storage_reciever_handle<float> ns_image_storage_handler::request_local_cache_storage_float(const std::string & filename, const ns_image_type & image_type, const unsigned long max_line_length, const bool report_to_db) const{

	std::string output_dir = volatile_storage_directory + DIR_CHAR_STR + ns_image_server_cache_directory(),
		output_filename = output_dir + DIR_CHAR_STR + filename;
	ns_dir::convert_slashes(output_filename);
	ns_dir::convert_slashes(output_dir);
	//make sure location is writeable
	ns_dir::create_directory_recursive(output_dir);
	if (!ns_dir::file_is_writeable(output_filename)) {
		ns_image_handler_submit_alert_to_central_db(ns_alert::ns_volatile_storage_error,
			"Could not access volatile storage",
			std::string("ns_image_storage_handler::request_local_cache_storage::Could not access volatile storage when attempting to write ") + filename);

		throw ns_ex("ns_image_storage_handler::request_local_cache_storage()::Volitile storage location is not writeable: ") << output_filename << ns_file_io;
	}
	if (verbosity > ns_standard && report_to_db) {
		ns_image_server_event ev("ns_image_storage_handler::Opening ");
		ev << output_filename << " for output.";
		ev.log = report_to_db;
		ns_image_handler_register_server_event_to_central_db(ev);
	}
	return ns_image_storage_reciever_handle<float>(new ns_image_storage_reciever_to_disk<float>(max_line_length, output_filename, image_type, true, output_file_permissions));
}


std::string ns_image_storage_handler::add_to_local_cache(ns_image_server_image & image, const ns_image_type & image_type, ns_image_server_sql * sql)const {

	ns_file_location_specification file_location(look_up_image_location(image, sql, image_type));

	if (!long_term_storage_is_accessible(file_location, __FILE__, __LINE__))
		throw ns_ex("Could not access long term storage.");

	try {
		string cache_filename = file_location.relative_directory + "_" + file_location.filename;
		std::string extension = ns_dir::extract_extension(file_location.filename);
		cache_filename = ns_dir::extract_filename_without_extension(cache_filename);
		if (file_location.relative_directory.size() == 0 || file_location.filename.size()==0)
			throw ns_ex("Blank image record encountered:");
		for (unsigned int i = 0; i < cache_filename.size(); i++)
			if (cache_filename[i] == '/' || cache_filename[i] == '\\') cache_filename[i] = '_';
		std::string dir = volatile_storage_directory + DIR_CHAR_STR + ns_image_server_cache_directory();
		ns_dir::convert_slashes(dir);
		ns_dir::create_directory_recursive(dir);
		std::string output_filename = dir + DIR_CHAR_STR + cache_filename;
		ns_dir::convert_slashes(output_filename);


		//guarentee unique filename for this process
		string tmp(output_filename);
		int i = 0;
		ns_64_bit thread_id = image_server_const.get_current_thread_state_info()->second.internal_thread_id;
		while(ns_dir::file_exists(tmp + "." + extension)){
			tmp = output_filename + "_" + ns_to_string(thread_id) + "_" + ns_to_string(i) ;
			i++;
		}
		if (i != 0) {
			cache_filename += "_" + ns_to_string(thread_id) + "_" + ns_to_string(i);
			output_filename += "_" + ns_to_string(thread_id) + "_" + ns_to_string(i);
		}
		cache_filename += "." + extension;
		output_filename += "." + extension;

		//	ns_imag_handler_register_server_event(ns_image_server_event("ns_image_storage_handler::Opening LT ",false) << display_filename << " for input." << ns_ts_minor_event);
		if (simulate_long_term_storage_errors || !ns_dir::copy_file(file_location.absolute_long_term_filename(), output_filename))
			throw ns_ex("Could not copy file ") << file_location.absolute_long_term_filename() << "\n to local cache location: " << output_filename;
		return cache_filename;


	}
	catch (ns_ex & ex) {

		*sql << "UPDATE " << sql->table_prefix() << "images SET problem=1 WHERE id = " << image.id;
		sql->send_query();
		throw ns_ex("ns_image_storage_handler::request_from_storage_n_bis()::file ") << image.path <<
			DIR_CHAR_STR << image.filename << " could not be opened: " << ex.text() << ns_file_io;
	}
}

ns_image_storage_source_handle<ns_image_storage_handler::ns_component> ns_image_storage_handler::request_from_local_cache(const std::string & filename, const bool report_to_db) const{
	if (!ns_dir::file_exists(volatile_storage_directory))
			ns_image_handler_submit_alert_to_central_db(ns_alert::ns_volatile_storage_error,
			"Could not access volatile storage",
			std::string("ns_image_storage_handler::request_from_local_cache::Could not access volatile storage when attempting to read ") + filename);

	std::string dir = volatile_storage_directory + DIR_CHAR_STR + ns_image_server_cache_directory();
	std::string input_filename = dir + DIR_CHAR_STR + filename;
	ns_dir::convert_slashes(input_filename);

	if (ns_dir::file_exists(input_filename)){
		if (verbosity > ns_standard && report_to_db)
			ns_image_handler_register_server_event_to_central_db(ns_image_server_event("ns_image_storage_handler::Opening ",false) << input_filename << " for input." << ns_ts_minor_event);
		return ns_image_storage_source_handle<ns_image_storage_handler::ns_component>(new ns_image_storage_source_from_disk<ns_image_storage_handler::ns_component>(input_filename,true));

	}
	throw ns_ex("ns_image_storage_handler::request_local_cache_storage()::Could not load file from cache: ") << filename << ns_file_io;
}

ns_image_storage_source_handle<float> ns_image_storage_handler::request_from_local_cache_float(const std::string & filename, const bool report_to_db) const{
	if (!ns_dir::file_exists(volatile_storage_directory))
		ns_image_handler_submit_alert_to_central_db(ns_alert::ns_volatile_storage_error,
			"Could not access volatile storage",
			std::string("ns_image_storage_handler::request_from_local_cache::Could not access volatile storage when attempting to read ") + filename);

	std::string dir = volatile_storage_directory + DIR_CHAR_STR + ns_image_server_cache_directory();
	std::string input_filename = dir + DIR_CHAR_STR + filename;
	ns_dir::convert_slashes(input_filename);

	if (ns_dir::file_exists(input_filename)) {
		if (verbosity > ns_standard && report_to_db)
			ns_image_handler_register_server_event_to_central_db(ns_image_server_event("ns_image_storage_handler::Opening ", false) << input_filename << " for input." << ns_ts_minor_event);
		return ns_image_storage_source_handle<float>(new ns_image_storage_source_from_disk<float>(input_filename, true));

	}
	throw ns_ex("ns_image_storage_handler::request_local_cache_storage()::Could not load file from cache: ") << filename << ns_file_io;
}


ns_image_storage_source_handle<ns_image_storage_handler::ns_component> ns_image_storage_handler::request_from_volatile_storage(const std::string & filename, const bool report_in_db) const{
	if (!ns_dir::file_exists(volatile_storage_directory))
		ns_image_handler_submit_alert_to_central_db(ns_alert::ns_volatile_storage_error,
			"Could not access volatile storage",
			std::string("ns_image_storage_handler::request_from_volatile_storage::Could not access volatile storage when attempting to read ") + filename);

	std::string dir = volatile_storage_directory + DIR_CHAR_STR + ns_image_server_scratch_directory();
	std::string input_filename = dir + DIR_CHAR_STR + filename;
	ns_dir::convert_slashes(input_filename);

	if (ns_dir::file_exists(input_filename)){
		if (verbosity > ns_standard && report_in_db)
			ns_image_handler_register_server_event_to_central_db(ns_image_server_event("ns_image_storage_handler::Opening VT ") << input_filename << " for input." << ns_ts_minor_event);
		return ns_image_storage_source_handle<ns_image_storage_handler::ns_component>(new ns_image_storage_source_from_disk<ns_image_storage_handler::ns_component>(input_filename,true));

	}
	throw ns_ex("ns_image_storage_handler::request_from_volatile_storage()::Could not load file from scratch: ") << input_filename << ns_file_io;
}


bool ns_image_storage_handler::delete_from_volatile_storage(const std::string & filename)const{
	if (!ns_dir::file_exists(volatile_storage_directory))
			ns_image_handler_submit_alert_to_central_db(ns_alert::ns_volatile_storage_error,
			"Could not access volatile storage",
			std::string("ns_image_storage_handler::delete_from_volatile_storage::Could not access volatile storage when attempting to delete ") + filename);

	std::string dir = volatile_storage_directory + DIR_CHAR_STR + ns_image_server_scratch_directory();
	std::string input_filename = dir + DIR_CHAR_STR + filename;
	ns_dir::convert_slashes(input_filename);
	if (verbosity >= ns_deletion_events)
		ns_image_handler_register_server_event_to_central_db(ns_image_server_event("Deleting file ") << input_filename);

	if (!ns_dir::file_exists(input_filename))
		return false;

	return ns_dir::delete_file(input_filename);
}
unsigned long ns_image_storage_handler::request_local_cache_file_size(const std::string & filename)const{
	if (!ns_dir::file_exists(volatile_storage_directory))
		ns_image_handler_submit_alert_to_central_db(ns_alert::ns_volatile_storage_error,
			"Could not access volatile storage",
			std::string("ns_image_storage_handler::request_local_cache_file_size::Could not access volatile storage when attempting to get size of ") + filename);

	std::string dir = volatile_storage_directory + DIR_CHAR_STR + ns_image_server_cache_directory();
	std::string input_filename = dir + DIR_CHAR_STR + filename;
	ns_dir::convert_slashes(input_filename);
	throw ns_ex("ns_image_storage_handler::request_local_cache_file_size::File size calculation not implemented!");
}
bool ns_image_storage_handler::delete_from_local_cache(const std::string & filename)const{
	if (!ns_dir::file_exists(volatile_storage_directory))
		ns_image_handler_submit_alert_to_central_db(ns_alert::ns_volatile_storage_error,
			"Could not access volatile storage",
			std::string("ns_image_storage_handler::delete_from_local_cache::Could not access volatile storage when attempting to delete ") + filename);

	std::string dir = volatile_storage_directory + DIR_CHAR_STR + ns_image_server_cache_directory();
	std::string input_filename = dir + DIR_CHAR_STR + filename;
	ns_dir::convert_slashes(input_filename);
	if (verbosity >= ns_deletion_events)
		image_server_const.register_server_event_no_db(ns_image_server_event("Deleting from cache ") << input_filename);
	ns_dir::delete_file(input_filename);
	return true;
}

void ns_image_storage_handler::clear_local_cache()const{
	if (!ns_dir::file_exists(volatile_storage_directory))
		image_server.register_server_event(ns_image_server::ns_register_in_central_db_with_fallback, ns_ex(
			"ns_image_storage_handler::clear_local_cache::Could not access volatile storage when attempting clear the local cache: ") << volatile_storage_directory);

	std::string path = volatile_storage_directory + DIR_CHAR_STR + ns_image_server_cache_directory();
	//make sure the path exists (so trying to load it does not fail;
	ns_dir::create_directory_recursive(path);
	ns_dir dir;
	dir.load(path);
	if (verbosity >= ns_deletion_events)
		image_server_const.register_server_event_no_db(ns_image_server_event("Clearing Local cache ") << path);
	for (unsigned long i = 0; i < (unsigned long)dir.files.size(); i++)
		if (dir.files[i].find(".txt") != dir.files[i].npos)
			ns_dir::delete_file(path + DIR_CHAR + dir.files[i]);
}


bool ns_image_storage_handler::delete_from_storage(ns_image_server_captured_image & image,const ns_file_deletion_type & type,ns_image_server_sql * sql)const{
	if (image.captured_images_id == 0)
		throw ns_ex("ns_image_storage_handler::delete_from_storage()::No id specified.");
	if (image.capture_images_image_id== 0)
		if(!image.load_from_db(image.captured_images_id,sql))
			throw ns_ex("ns_image_storage_handler::delete_from_storage()::Could not delete non-existant captured image") << ns_file_io;
	ns_image_server_image im;
	im.id = image.capture_images_image_id;
	delete_from_storage(im,type,sql);
	return true;
}

void ns_image_storage_handler::delete_file_specification(const ns_file_location_specification & spec,const ns_file_deletion_type & type)const{
	if (spec.long_term_directory != long_term_storage_directory)
		throw ns_ex("ns_image_storage_handler::delete_directory()::Attempting to delete ouside of long term storage directory: ") << spec.absolute_long_term_directory();
	if (spec.volatile_directory!= volatile_storage_directory)
		throw ns_ex("ns_image_storage_handler::delete_directory()::Attempting to delete ouside of volatile storage directory:") << spec.absolute_volatile_directory();

	if (spec.filename.size() == 0){
		if (verbosity >= ns_standard)
			ns_image_handler_register_server_event_to_central_db(ns_image_server_event("Deleting Folder ") << spec.relative_directory);
		if ((type == ns_delete_volatile || type == ns_delete_both_volatile_and_long_term) &&
			 ns_dir::file_exists(spec.absolute_volatile_directory())){
			if (!ns_dir::delete_folder_recursive(spec.absolute_volatile_directory()))
				throw ns_ex("ns_image_storage_handler::delete_directory()::Could not delete directory ") << spec.absolute_volatile_directory() << ns_file_io;
		}
		if (!simulate_long_term_storage_errors && (type == ns_delete_long_term || type == ns_delete_both_volatile_and_long_term) &&
			 ns_dir::file_exists(spec.absolute_long_term_directory())){
			if (!ns_dir::delete_folder_recursive(spec.absolute_long_term_directory()))
				throw ns_ex("ns_image_storage_handler::delete_directory()::Could not delete directory ") << spec.absolute_long_term_directory()<< ns_file_io;
		}
	}
	else{
		if ((type == ns_delete_volatile || type == ns_delete_both_volatile_and_long_term) &&
			ns_dir::file_exists(spec.absolute_volatile_filename())){
			if (verbosity >= ns_deletion_events)
				ns_image_handler_register_server_event_to_central_db(ns_image_server_event("Deleting file ") << spec.absolute_volatile_filename());
			if (!ns_dir::delete_file(spec.absolute_volatile_filename()))
				throw ns_ex("ns_image_storage_handler::delete_directory()::Could not delete file") << spec.absolute_volatile_filename() << ns_file_io;

		}
		if (!simulate_long_term_storage_errors && (type == ns_delete_long_term || type == ns_delete_both_volatile_and_long_term) &&
			ns_dir::file_exists(spec.absolute_long_term_filename())){
			if (verbosity >= ns_deletion_events)
				ns_image_handler_register_server_event_to_central_db(ns_image_server_event("Deleting file ") << spec.absolute_long_term_filename());
			if (!ns_dir::delete_file(spec.absolute_long_term_filename()))
				throw ns_ex("ns_image_storage_handler::delete_directory()::Could not delete file ") << spec.absolute_long_term_filename() << ns_file_io;
		}
	}
}


bool ns_image_storage_handler::delete_from_storage(ns_image_server_image & image,const ns_file_deletion_type & type,ns_image_server_sql * sql)const{

	bool successfully_deleted(false);
	ns_file_location_specification file_location(look_up_image_location_no_extension_alteration(image,sql));
	if (type == ns_delete_volatile || type == ns_delete_both_volatile_and_long_term){
		if (verbosity >= ns_deletion_events)
			image_server_const.register_server_event(ns_image_server_event("ns_image_storage_handler::delete_from_storage(ns_image_server_image & image)::Deleting ") << file_location.absolute_volatile_filename(),sql);

		successfully_deleted = ns_dir::delete_file(file_location.absolute_volatile_filename());
		if (type == ns_delete_volatile && !successfully_deleted)
			image_server_const.register_server_event(ns_image_server_event("Could not delete ") << file_location.absolute_volatile_filename(),sql);
		if (type != ns_delete_long_term && type != ns_delete_both_volatile_and_long_term)
			return false;
	}

	if (!simulate_long_term_storage_errors && (type == ns_delete_long_term || type == ns_delete_both_volatile_and_long_term) &&
		long_term_storage_directory.size() != 0 && ns_dir::file_exists(file_location.long_term_directory)){

		if (verbosity >= ns_deletion_events)
			image_server_const.register_server_event(ns_image_server_event("ns_image_storage_handler::delete_from_storage(ns_image_server_image & image)::Deleting ") << file_location.absolute_long_term_filename(),sql);

		successfully_deleted = successfully_deleted || ns_dir::delete_file(file_location.absolute_long_term_filename());
		if (!successfully_deleted){
			image_server_const.register_server_event(ns_image_server_event("Could not delete ") << file_location.absolute_long_term_filename(),sql);
			return false;
		}
		//ns_image_handler_register_server_event(ns_image_server_event("ns_image_storage_handler::Deleting (LT) ") << image.filename << ns_ts_minor_event);
		return true;
	}
	ns_image_handler_submit_alert_to_central_db(ns_alert::ns_long_term_storage_error,
			"Could not access long term storage storage",
			std::string("ns_image_storage_handler::delete_from_storage::Could not access volatile storage when attempting to delete ") + file_location.relative_directory + DIR_CHAR_STR + file_location.filename);

	//if we have no access to the long-term storage, we have to ask a node that does to delete the file.
	/*try{
		network_lock.wait_to_acquire(__FILE__,__LINE__);
		ns_socket_connection con = connect_to_fileserver_node(sql);
		ns_image_server_message message(con);
		message.send_message(NS_DELETE_IMAGE,image.id);
		con.close();
		ns_image_handler_register_server_event(ns_image_server_event("ns_image_storage_handler::Requesting Network Deletion of ") << image.filename << ns_ts_minor_event);
	}
	catch(...){
		network_lock.release();
		throw;
	}*/
	return false;
}

ns_image_storage_source_handle<ns_image_storage_handler::ns_component> ns_image_storage_handler::request_from_storage(ns_image_server_captured_image_region & region_image, ns_image_server_sql * sql)const{
	ns_image_server_image im;
	if (region_image.region_images_image_id == 0)
		region_image.load_from_db(region_image.region_images_id,sql);
	im.load_from_db(region_image.region_images_image_id,sql);
	return request_from_storage(im,sql);
}
ns_image_storage_source_handle<ns_image_storage_handler::ns_component> ns_image_storage_handler::request_from_storage(ns_image_server_captured_image_region & region_image, const ns_processing_task & task, ns_image_server_sql * sql)const{
	ns_image_server_image im;
	if (!sql->connected_to_central_database())
		throw ns_ex("ns_image_storage_handler::request_from_storage(ns_image_server_captured_image_region)::Not connected to central db!");
	im = region_image.request_processed_image(task,*static_cast<ns_sql *>(sql));
//	im.load_from_db(region_image.region_images_image_id,sql);
	return request_from_storage(im,sql);
}
ns_image_storage_source_handle<ns_image_storage_handler::ns_component> ns_image_storage_handler::request_from_storage(ns_image_server_image & image, const ns_processing_task & task, ns_image_server_sql * sql)const{
	throw ns_ex("ns_image_storage_handler::request_from_storage(ns_image_server_image)::Cannot produce task from image.");
}

ns_image_storage_source_handle<ns_image_storage_handler::ns_component> ns_image_storage_handler::request_from_storage(ns_image_server_image & image, ns_image_server_sql * sql)const{
	return request_from_storage_n_bits<ns_image_storage_handler::ns_component>(image, sql,ns_volatile_and_long_term_storage);
}

ns_image_storage_source_handle<ns_image_storage_handler::ns_component> ns_image_storage_handler::request_from_storage(ns_image_server_captured_image & captured_image, ns_image_server_sql * sql)const{
	ns_image_server_image im;
	im.id = captured_image.capture_images_image_id;
	//cerr << "loading image info...";
	im.load_from_db(im.id,sql);
//	im.load_from_db(region_image.region_images_image_id,sql);
//	cerr << "loading image...";

	return request_from_storage(im,sql);
}


void ns_image_storage_handler::set_experiment_partition_cache_update_period(unsigned long seconds){
	experiment_partition_cache_update_period = seconds;
}

void ns_image_storage_handler::move_experiment_partition(ns_64_bit experiment_id, const std::string & partition, ns_sql & sql){
	experiment_partition_cache_lock.wait_to_acquire(__FILE__,__LINE__);
	try{
		ns_move_experiment_partition(experiment_id,partition,sql);
		experiment_partition_cache_last_update_time = 0;
		refresh_experiment_partition_cache_int(&sql,false);
		experiment_partition_cache_lock.release();
	}
	catch(...){
		experiment_partition_cache_lock.release();
		throw;
	}
}

void ns_image_storage_handler::refresh_experiment_partition_cache(ns_image_server_sql * sql){
	unsigned long current_time = ns_current_time();
	if (current_time - experiment_partition_cache_last_update_time >= experiment_partition_cache_update_period)
		refresh_experiment_partition_cache_int(sql,true);
}
bool ns_image_storage_handler::move_file(const ns_file_location_specification & source, const ns_file_location_specification & dest,bool volatile_storage) {
	if (volatile_storage)
		return ns_dir::move_file(source.absolute_volatile_filename(), dest.absolute_volatile_filename());
	else
		return ns_dir::move_file(source.absolute_long_term_filename(), dest.absolute_long_term_filename());
}
ns_file_location_specification ns_image_storage_handler::get_file_specification_for_image(ns_image_server_image & image,ns_image_server_sql * sql) const{
	return this->look_up_image_location_no_extension_alteration(image,sql);
}

std::string ns_image_storage_handler::movement_file_directory(ns_64_bit region_info_id, ns_image_server_sql * sql, std::string & absolute_directory_prefix) const {

	std::string region_name,sample_name,experiment_name,dir;
	ns_64_bit sample_id,experiment_id;
	ns_region_info_lookup::get_region_info(region_info_id,sql,region_name,sample_name,sample_id,experiment_name,experiment_id);
	std::string experiment_directory(ns_image_server_captured_image::experiment_directory(experiment_name,experiment_id));

	dir = ns_sample_directory(sample_name,sample_id,experiment_directory);
	dir+= DIR_CHAR_STR;
	dir+= "movement_data";
	absolute_directory_prefix = long_term_storage_directory + DIR_CHAR_STR + get_partition_for_experiment_int(experiment_id, sql);
	return dir;
}

ns_file_location_specification ns_image_storage_handler::get_storyboard_path(const ns_64_bit & experiment_id, const ns_64_bit & region_id, const ns_64_bit & subimage_id, const std::string & filename_suffix, const ns_image_type & image_type, ns_sql & sql, const bool just_path) const{
	ns_file_location_specification spec;
	if (experiment_id != 0 && region_id != 0)
		throw ns_ex("Storyboards for experiments and regions are stored in different locations! Specify only one.");
	if (region_id != 0) {
		sql << "SELECT r.name,s.name, s.id,e.name,e.id FROM sample_region_image_info as r, capture_samples as s, experiments as e WHERE "
			"r.id = " << region_id << " AND r.sample_id = s.id AND s.experiment_id = e.id";
		ns_sql_result res;
		sql.get_rows(res);
		if (res.size() == 0)
			throw ns_ex("ns_experiment_storyboard_manager::get_default_storage_locations()::Could not load information for region ") << region_id;

		const std::string experiment_directory(ns_image_server_captured_image_region::experiment_directory(res[0][3], ns_atoi64(res[0][4].c_str())));
		std::string region_path = ns_image_server_captured_image_region::region_base_directory(res[0][0],
			ns_image_server_captured_image_region::captured_image_directory_d(res[0][1], ns_atoi64(res[0][2].c_str()), experiment_directory, false),
			experiment_directory);
		std::string f;
		if (!just_path) {
			spec.filename = res[0][3] + "=" + res[0][1] +
				"=" + res[0][0] + "=" + filename_suffix + "=" + ns_to_string(subimage_id);
		}
		ns_add_image_suffix(spec.filename, image_type);
		spec.relative_directory = region_path + DIR_CHAR_STR + "animal_storyboard";

		spec.partition = image_server.image_storage.get_partition_for_experiment(ns_atoi64(res[0][4].c_str()), &sql);
		return spec;
	}
	if (experiment_id == 0)
		throw ns_ex("No samples specified");

	sql << "SELECT name FROM experiments WHERE id = " << experiment_id;
	ns_sql_result res;
	sql.get_rows(res);
	if (res.size() == 0)
		throw ns_ex("ns_experiment_storyboard_manager::get_default_storage_locations()::Could not load information for experiment ") << experiment_id;
	if (!just_path)
		spec.filename = res[0][0] + "=" + filename_suffix + "=" + ns_to_string(subimage_id);
	const std::string experiment_directory(ns_image_server_captured_image_region::experiment_directory(res[0][0], experiment_id));
	ns_add_image_suffix(spec.filename, image_type);
	spec.relative_directory = experiment_directory + DIR_CHAR_STR + "animal_storyboard";
	spec.partition = image_server.image_storage.get_partition_for_experiment(experiment_id, &sql);
	return spec;
}

ns_file_location_specification  ns_image_storage_handler::get_detection_data_path_for_region(ns_64_bit region_image_info_id, ns_image_server_sql * sql) const {
	ns_file_location_specification spec(get_base_path_for_region(region_image_info_id, sql));
	spec.relative_directory += DIR_CHAR;
	spec.relative_directory += "detected_data";
	return spec;
}
ns_file_location_specification ns_image_storage_handler::convert_results_file_to_location(const ns_image_server_results_file * f) {
	ns_file_location_specification spec;
	spec.partition = "";
	spec.long_term_directory = f->long_term_directory;
	spec.relative_directory = f->relative_directory;
	spec.filename = f->filename;
	return spec;
}

ns_file_location_specification ns_image_storage_handler::get_file_specification_for_movement_data(ns_64_bit region_info_id, const std::string & data_source,ns_image_server_sql * sql) const{
	ns_file_location_specification spec;
	std::string abs_path;

	spec.relative_directory = movement_file_directory(region_info_id,sql, abs_path);

	std::string region_name,sample_name,experiment_name;
	ns_64_bit sample_id,experiment_id;
	ns_region_info_lookup::get_region_info(region_info_id,sql,region_name,sample_name,sample_id,experiment_name,experiment_id);
	spec.partition = get_partition_for_experiment_int(experiment_id,sql);
	spec.filename = experiment_name + "=" + sample_name + "=" + region_name;
	
	if (data_source.size() > 0)
		spec.filename += std::string("=") + data_source;
	ns_add_image_suffix(spec.filename, ns_csv);
	return spec;
}


ns_image_server_image ns_image_storage_handler::get_region_movement_metadata(ns_64_bit region_info_id, const std::string& metadata_type, ns_sql& sql) const {
	ns_file_location_specification spec(get_file_specification_for_movement_data(region_info_id, metadata_type, &sql));
	ns_image_server_image im;
	im.filename = spec.filename;
	im.partition = spec.partition;
	im.path = spec.relative_directory;
	return im;
}
#ifndef NS_ONLY_IMAGE_ACQUISITION
ns_time_path_movement_result_files ns_image_storage_handler::get_region_movement_quantification_metadata(ns_64_bit region_info_id,ns_sql & sql) const{
	ns_time_path_movement_result_files files;
	ns_image_server_image im(get_region_movement_metadata(region_info_id, files.base_name(), sql));
	im.filename = ns_dir::extract_filename_without_extension(im.filename);
	files.set_from_base_db_record(im);
	files.only_quantification_specified = false;
	
	return files;
}

#endif
ns_file_location_specification  ns_image_storage_handler::get_file_specification_for_path_data(const ns_file_location_specification & region_spec) const{
	ns_file_location_specification spec(region_spec);
	spec.relative_directory += DIR_CHAR_STR;
	spec.relative_directory+="path_data";
	return spec;
}

ns_image_server_image ns_image_storage_handler::get_storage_for_specification(const ns_file_location_specification & spec) {
	ns_image_server_image im;
	im.filename = spec.filename;
	im.path = spec.relative_directory;
	im.partition = spec.partition;
	return im;
}

ns_image_server_image ns_image_storage_handler::get_storage_for_path(const ns_file_location_specification & region_spec,
			const unsigned long path_id, const unsigned long path_group_id,
			const ns_64_bit region_info_id, const std::string & region_name, const std::string & experiment_name, const std::string & sample_name,bool flow) const{

	ns_file_location_specification spec(region_spec);
	ns_file_location_specification dir_spec(get_file_specification_for_path_data(region_spec));
	ns_image_server_image im;
	im.partition = spec.partition;
	im.host_id = image_server_const.host_id();
	im.path = dir_spec.relative_directory;
	im.filename = experiment_name + "=" + sample_name +"=" + region_name + "=" + ns_to_string(region_info_id) + "="
			+ ns_to_string(path_group_id) + "=" + ns_to_string(path_id);
	if (flow)
		im.filename += "=flow";
	ns_add_image_suffix(im.filename,ns_tiff);
	return im;
}

ns_file_location_specification ns_image_storage_handler::get_base_path_for_region(ns_64_bit region_image_info_id,ns_image_server_sql * sql) const{
	std::string region_name,sample_name,experiment_name;
	ns_64_bit sample_id,experiment_id;
	ns_region_info_lookup::get_region_info(region_image_info_id,sql,region_name,sample_name,sample_id,experiment_name,experiment_id);
	std::string experiment_directory(ns_image_server_captured_image::experiment_directory(experiment_name,experiment_id));

	std::string relative_path =
		ns_image_server_captured_image_region::region_base_directory(region_name,
			ns_sample_directory(sample_name,sample_id,experiment_directory),experiment_directory);
	std::string partition(get_partition_for_experiment_int(experiment_id,sql));
	return compile_absolute_paths_from_relative(relative_path,partition,"");
}
ns_file_location_specification ns_image_storage_handler::get_path_for_region(ns_64_bit region_image_info_id,ns_image_server_sql * sql, const ns_processing_task task) const{
	std::string region_name,sample_name,experiment_name;
	ns_64_bit sample_id,experiment_id;
	ns_region_info_lookup::get_region_info(region_image_info_id,sql,region_name,sample_name,sample_id,experiment_name,experiment_id);
	std::string experiment_directory(ns_image_server_captured_image::experiment_directory(experiment_name,experiment_id));

	std::string relative_path =
		ns_image_server_captured_image_region::region_directory(region_name,sample_name,
			ns_sample_directory(sample_name,sample_id,experiment_directory),experiment_directory,task);
	std::string partition(get_partition_for_experiment_int(experiment_id,sql));
	return compile_absolute_paths_from_relative(relative_path,partition,"");
}

ns_file_location_specification ns_image_storage_handler::get_path_for_sample_captured_images(ns_64_bit sample_id, bool small_images, ns_image_server_sql * sql) const {
	ns_file_location_specification spec(get_path_for_sample(sample_id,sql));
	spec.relative_directory += DIR_CHAR_STR;

	if (small_images)
		 spec.relative_directory +="captured_small_images";
	else spec.relative_directory +="captured_images";

	return compile_absolute_paths_from_relative(spec.relative_directory,spec.partition,"");
}

ns_file_location_specification ns_image_storage_handler::get_path_for_sample(ns_64_bit sample_id, ns_image_server_sql * sql) const{
	std::string sample_name,experiment_name;
	ns_64_bit experiment_id;
	ns_region_info_lookup::get_sample_info(sample_id,sql,sample_name,experiment_name,experiment_id);

	std::string relative_path =
			ns_sample_directory(sample_name,sample_id,
				ns_image_server_captured_image::experiment_directory(experiment_name,experiment_id));

	std::string partition(get_partition_for_experiment_int(experiment_id,sql));
	return compile_absolute_paths_from_relative(relative_path,partition,"");
}

ns_file_location_specification ns_image_storage_handler::get_path_for_video_storage(ns_64_bit experiment_id, ns_image_server_sql * sql) const{
	std::string experiment_name;
	ns_region_info_lookup::get_experiment_info(experiment_id,sql,experiment_name);

	std::string relative_path =
				ns_image_server_captured_image::experiment_directory(experiment_name,experiment_id);

	std::string partition(get_partition_for_experiment_int(experiment_id,sql));
	return compile_absolute_paths_from_relative(relative_path,partition,"");
}

ns_file_location_specification ns_image_storage_handler::get_path_for_experiment(ns_64_bit experiment_id, ns_image_server_sql * sql) const{
	std::string experiment_name;
	ns_region_info_lookup::get_experiment_info(experiment_id,sql,experiment_name);

	std::string relative_path =
				ns_image_server_captured_image::experiment_directory(experiment_name,experiment_id);

	std::string partition(get_partition_for_experiment_int(experiment_id,sql));
	return compile_absolute_paths_from_relative(relative_path,partition,"");
}

ns_64_bit ns_image_storage_handler::create_file_deletion_job(const ns_64_bit parent_processing_job_id, ns_sql & sql)const{
	sql << "INSERT INTO delete_file_jobs SET parent_job_id = " << parent_processing_job_id<< ", confirmed=0";
	return sql.send_query_get_id();
}
void ns_image_storage_handler::delete_file_deletion_job(const ns_64_bit deletion_job_id, ns_sql & sql)const{
	sql << "DELETE FROM delete_file_specifications WHERE delete_job_id = " << deletion_job_id;
	sql.send_query();
	sql << "DELETE FROM delete_file_jobs WHERE id = " << deletion_job_id;
	sql.send_query();
}
void ns_image_storage_handler::submit_file_deletion_request(const ns_64_bit deletion_job_id,const ns_file_location_specification & spec, ns_sql & sql)const{
	sql << "INSERT INTO delete_file_specifications SET delete_job_id = " << deletion_job_id << ","
		<< "relative_directory = '" << sql.escape_string(spec.relative_directory) << "', filename = '" << sql.escape_string(spec.filename) << "'"
		<< ",`partition`='" << sql.escape_string(spec.partition) << "'";
	sql.send_query();
	sql.send_query("COMMIT");
}
void ns_image_storage_handler::get_file_deletion_requests(const ns_64_bit deletion_job_id, ns_64_bit & parent_job_id,std::vector<ns_file_location_specification> & specs, ns_sql & sql)const{
	sql << "SELECT parent_job_id,confirmed FROM delete_file_jobs WHERE id = " << deletion_job_id;
	ns_sql_result res;
	sql.get_rows(res);
	if (res.size() == 0)
		throw ns_ex("ns_handle_file_delete_action()::Specified file deletion job could not be found in the database: ") << deletion_job_id;
	if (res[0][0] == "0")
		throw ns_ex("ns_handle_file_delete_action()::Parent processeing job has not been specified");
	if (res[0][1] == "0")
		throw ns_ex("ns_handle_file_delete_action()::Specified file deletion job has not been confirmed.");
	parent_job_id = ns_atoi64(res[0][0].c_str());
	sql << "SELECT relative_directory,filename,`partition` FROM delete_file_specifications WHERE delete_job_id = " << deletion_job_id;
	sql.get_rows(res);
	specs.resize(0);
	specs.resize(res.size());

	for (unsigned int i = 0; i < res.size(); i++){
		specs[i].long_term_directory = long_term_storage_directory;
		specs[i].volatile_directory = volatile_storage_directory;
		specs[i].relative_directory = res[i][0];
		//convert all slashes to local file system appropriate
		for (unsigned int j = 0; j < specs[i].relative_directory.size(); j++) {
			if (specs[i].relative_directory[j] == WRONG_DIR_CHAR)
				specs[i].relative_directory[j] = DIR_CHAR;
		}
		specs[i].filename = res[i][1];
		specs[i].partition = res[i][2];
	}
}
std::string ns_get_video_path_specifics(const bool for_sample_video, const bool only_base){
	string r(ns_processing_step_directory_d(ns_process_compile_video));
	if (only_base)
		return r;
	r+= DIR_CHAR_STR;
	if (for_sample_video)
		r+= "samples";
	else r+= "regions";
	return r;
}
std::string ns_image_storage_handler::get_relative_path_for_video(const ns_file_location_specification & spec, const bool for_sample_video, const bool only_base)const{
	return spec.relative_directory + DIR_CHAR_STR + ns_get_video_path_specifics(for_sample_video,only_base);
}
std::string ns_image_storage_handler::get_absolute_path_for_video(const ns_file_location_specification & spec, const bool for_sample_video, const bool only_base)const{
	return spec.absolute_long_term_directory() + DIR_CHAR_STR + ns_get_video_path_specifics(for_sample_video,only_base);
}
std::string ns_image_storage_handler::get_absolute_path_for_video_image(const ns_64_bit experiment_id, const std::string & rel_path, const std::string & filename,ns_sql & sql)const{
	std::string partition(get_partition_for_experiment_int(experiment_id,&sql));
	ns_file_location_specification spec(compile_absolute_paths_from_relative(rel_path,partition,filename));
	return spec.absolute_long_term_directory() + DIR_CHAR_STR + filename;
}
void ns_region_info_lookup::get_region_info(const ns_64_bit region_id,ns_image_server_sql * sql, std::string & region_name, std::string & sample_name,ns_64_bit & sample_id, std::string & experiment_name, ns_64_bit & experiment_id){
	*sql << "SELECT sample_id,name FROM " << sql->table_prefix() << "sample_region_image_info WHERE id = " << region_id;
	ns_sql_result res;
	sql->get_rows(res);
	if (res.size() == 0)
		throw ns_ex("ns_image_storage_handler::get_region_info()::Could not locate region id ") << region_id << " in the database.";
	region_name = res[0][1];
	sample_id = ns_atoi64(res[0][0].c_str());
	get_sample_info(sample_id,sql,sample_name,experiment_name,experiment_id);
}
void ns_region_info_lookup::get_sample_info(const ns_64_bit sample_id,ns_image_server_sql * sql, std::string & sample_name, std::string & experiment_name, ns_64_bit & experiment_id){
	*sql << "SELECT name, experiment_id FROM " << sql->table_prefix() << "capture_samples WHERE id = " << sample_id;
	ns_sql_result res;
	sql->get_rows(res);
	if (res.size() == 0)
		throw ns_ex("ns_image_storage_handler::get_sample_info()::Could not locate sample id ") << sample_id << " in the database.";
	sample_name = res[0][0];
	experiment_id = ns_atoi64(res[0][1].c_str());
	get_experiment_info(experiment_id,sql,experiment_name);
}
void ns_region_info_lookup::get_experiment_info(const ns_64_bit experiment_id,ns_image_server_sql * sql, std::string & experiment_name){
	*sql << "SELECT name FROM " << sql->table_prefix() << "experiments WHERE id = " << experiment_id;
	ns_sql_result res;
	sql->get_rows(res);
	if (res.size() == 0)
		throw ns_ex("ns_image_storage_handler::get_experiment_info()::Could not locate experiment id ") << experiment_id << " in the database.";
	experiment_name = res[0][0];
}

ns_file_location_specification ns_image_storage_handler::look_up_image_location_no_extension_alteration(ns_image_server_image & image, ns_image_server_sql * sql) const {
	return look_up_image_location(image, sql, ns_unknown, false);
}
ns_file_location_specification ns_image_storage_handler::look_up_image_location(ns_image_server_image & image, ns_image_server_sql * sql, const ns_image_type & image_type, const bool alter_file_extension) const {
	ns_file_location_specification spec;
	if ((image.filename.size() == 0 || image.path.size() == 0) && image.id <= 0)
		throw ns_ex("ns_image_storage_handler::look_up_image_location()::No id specified when requesting image storage.");

	if (image.filename.size() == 0 || image.path.size() == 0)
		image.load_from_db(image.id, sql);
	if (image.filename.empty())
		throw ns_ex("ns_image_storage_handler::Empty filename requested");
	if (alter_file_extension) {
		std::string desired_suffix;
		ns_add_image_suffix(desired_suffix, image_type);
		if (ns_dir::extract_extension(image.filename) != desired_suffix.substr(1))
			throw ns_ex("Requesting incorrect suffix: ") << image.filename << " != " << desired_suffix.substr(1);
	}
	ns_dir::convert_slashes(image.path);
	return compile_absolute_paths_from_relative(image.path,image.partition,image.filename);
}


ns_file_location_specification ns_image_storage_handler::compile_absolute_paths_from_relative(const std::string & rel_path, const std::string & partition, const std::string & filename) const{
	ns_file_location_specification spec;
	spec.long_term_directory = long_term_storage_directory;
	spec.volatile_directory = volatile_storage_directory;
	spec.partition = partition;
	spec.relative_directory = rel_path;
	spec.filename = filename;
	return spec;
}

void ns_image_storage_handler::refresh_experiment_partition_cache_int(ns_image_server_sql * sql,const bool get_lock) const{
	*sql << "SELECT id,`partition` FROM " << sql->table_prefix() << "experiments";
	ns_sql_result res;
	sql->get_rows(res);

	if (get_lock) experiment_partition_cache_lock.wait_to_acquire(__FILE__,__LINE__);
	try{
		experiment_partition_cache.clear();
		for (unsigned int i = 0; i < res.size(); i++){
			std::string partition = res[i][1];
			if (partition.size() == 0)
				partition = ns_get_default_partition();
			experiment_partition_cache[ns_atoi64(res[i][0].c_str())] = partition;
		}
		if (get_lock) experiment_partition_cache_lock.release();
		experiment_partition_cache_last_update_time = ns_current_time();
	}
	catch(...){
		if (get_lock) experiment_partition_cache_lock.release();
		throw;

	}

}

std::string ns_image_storage_handler::get_partition_for_experiment_int(const ns_64_bit experiment_id, ns_image_server_sql * sql,bool request_from_db_on_miss, const bool get_lock) const{
	if (get_lock)
		experiment_partition_cache_lock.wait_to_acquire(__FILE__,__LINE__);
	try{
		map<ns_64_bit,std::string>::iterator p = experiment_partition_cache.find(experiment_id);
		if (p == experiment_partition_cache.end()){
			if (!request_from_db_on_miss)
				throw ns_ex("ns_image_storage_handler::get_partition_for_experiment_int()::Experiment id ") << experiment_id << " does not have an entry in the local experiment_partition_cache!";
			if (sql==0)
				throw ns_ex("ns_image_storage_handler::get_partition_for_experiment_int()::Requesting an experiment partition without a database connection");
				refresh_experiment_partition_cache_int(sql,false);

			std::string tmp(get_partition_for_experiment_int(experiment_id,sql,false,false));
			if (get_lock)
				experiment_partition_cache_lock.release();
			return tmp;
		}
		std::string a = p->second;
		if (get_lock)
			experiment_partition_cache_lock.release();
		if (a.size() == 0)
			ns_image_handler_register_server_event(ns_image_server_event("Warning: Experiment id ") << experiment_id << " has a blank partition specified in the database.",sql);
		return a;
	}
	catch(...){
		if (get_lock)
			experiment_partition_cache_lock.release();
		throw;
	}
}

std::string ns_shorten_filename(std::string name, const unsigned long limit){
		if(name.length() <= limit){
		return name;
	}
	else{
		std::string n;
		for (unsigned int i = 0; i < name.size(); i++){
			if (name[i] != 'a' &&
				name[i] != 'e' &&
				name[i] != 'i' &&
				name[i] != 'o' &&
				name[i] != 'u')
				n+=name[i];
		}
		//if the string is still too long after removing vowels, crop it and add a hash to the end
		std::string h(ns_to_string(std::hash<std::string>{}(name)));
		long p((long)limit - (long)h.size());
		if (p > 0)
			n.resize(p);
		n += h;

		return n;
	}
}

ns_socket_connection ns_image_storage_handler::connect_to_fileserver_node(ns_sql & sql){
	//get a list of all hosts able to recieve files and save them to long-term storage.
	sql << "SELECT ip, port FROM hosts WHERE id!='" << ns_image_storage_handler_host_id() << "' AND long_term_storage_enabled=1 AND "
		<< "abs(" << ns_current_time() << " - current_time ) < " << ns_image_storage_handler_server_timeout_interval();
	ns_sql_result hosts;
	sql.get_rows(hosts);

	if (hosts.size() == 0)
		throw ns_ex("ns_image_storage_handler::The cluster has no online long-term storage hosts!");

	//pick a random order in which to try hosts.
	//this should help distribute the load among all hosts.
	std::vector<unsigned int> order(hosts.size());
	for (unsigned int i = 0; i < hosts.size(); i++)
		order[i] = i;
	std::random_device rd;
	std::mt19937 g(rd());
	std::shuffle(order.begin(), order.end(),g);


	for (unsigned int i = 0; i < hosts.size(); i++){
		try{
			return socket.connect(hosts[ order[i] ][0], atol(hosts[ order[i] ][1].c_str()));
		}
		catch(ns_ex & ex){/*do nothing */}
	}

	throw ns_ex("ns_image_storage_handler::All ") << (unsigned int)hosts.size() << " long-term storage hosts failed to respond.";
}
