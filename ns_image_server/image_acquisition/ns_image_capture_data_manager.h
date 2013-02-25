#ifndef NS_CAPTURE_DATA_MANAGER_H
#define NS_CAPTURE_DATA_MANAGER_H
#include "ns_image_storage_handler.h"
#include "ns_capture_device.h"
#include "ns_single_thread_coordinator.h"

class ns_image_capture_data_manager{
public:
	typedef enum{ns_not_finished,ns_on_local_server_in_16bit,ns_on_local_server_in_8bit,ns_transferred_to_long_term_storage} ns_capture_image_status;

	ns_image_capture_data_manager(ns_image_storage_handler & storage_handler_):check_sql_lock("icdm::sql"),check_sql(0),device_transfer_state_lock("icdm::dev"),storage_handler(&storage_handler_),pending_transfers_lock("ns_icd::transfer"){}
	
	void initialize_capture_start(ns_image_capture_specification & capture_specification, ns_local_buffer_connection & local_buffer_sql);

	void register_capture_stop(ns_image_capture_specification & capture_specification, const ns_64_bit problem_id, ns_local_buffer_connection & local_buffer_sql);
	
	void transfer_image_to_long_term_storage(const std::string & device_name,unsigned int capture_schedule_entry_id, ns_image_server_captured_image & image, ns_sql & sql);

	bool handle_pending_transfers_to_long_term_storage(const std::vector<std::string> & device_names);
	
	bool handle_pending_transfers_to_long_term_storage_using_db_names();

	void wait_for_transfer_finish();
	~ns_image_capture_data_manager();
private:

	ns_sql * check_sql;
	bool transfer_in_progress_for_device(const std::string & device);

	void transfer_image_to_long_term_storage_locked(unsigned int capture_schedule_entry_id, ns_image_server_captured_image & image, ns_sql & sql);

	static ns_thread_return_type thread_start_handle_pending_transfers_to_long_term_storage(void * thread_arguments);

	unsigned long handle_pending_transfers(const std::string & device_name);

	bool transfer_data_to_long_term_storage(ns_image_server_captured_image & image,
									unsigned long long & time_during_transfer_to_long_term_storage,
									unsigned long long & time_during_deletion_from_local_storage,
									ns_sql & sql);

	ns_single_thread_coordinator pending_transfers_thread;
	ns_lock pending_transfers_lock;
	ns_image_storage_handler * storage_handler;
	bool device_transfer_in_progress(const std::string & name);
	void set_device_transfer_state(const bool state,const std::string & name);
	std::map<std::string, bool> device_transfer_state;
	ns_lock device_transfer_state_lock;
	ns_lock check_sql_lock;
};
#endif
