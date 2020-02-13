#ifndef NS_experiment_storyboard_annotater_H
#define NS_experiment_storyboard_annotater_H
#include "ns_image.h"
#include "ns_image_server.h"
#include "ns_image_series_annotater.h"
#include "ns_death_time_annotation.h"
#include "ns_experiment_storyboard.h"
#include "ns_time_path_image_analyzer.h"
#include "ns_population_telemetry.h"


//triangle drawing from http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
void ns_fill_bottom_flat_triangle(const ns_vector_2i & t1, const ns_vector_2i & b2, const ns_vector_2i & b3,const ns_color_8 &c,const double & opacity,ns_image_standard & im);
void ns_fill_top_flat_triangle(const ns_vector_2i & t1, const ns_vector_2i & t2, const ns_vector_2i & b3,const ns_color_8 &c,const double & opacity,ns_image_standard & im);


void ns_launch_worm_window_for_worm(const ns_64_bit region_id, const ns_stationary_path_id & worm, const unsigned long current_time);

class ns_experiment_storyboard_annotater;
class ns_experiment_storyboard_annotater_timepoint : public ns_annotater_timepoint{
private:
	 ns_image_storage_source_handle<ns_8_bit> get_image(ns_sql & sql){
		throw ns_ex("Data is stored in memory");
	 }
public:
	void init(){
		blacked_out_non_subject_animals = false;
	}
	ns_experiment_storyboard_annotater * experiment_annotater;
	ns_image_standard division_image;
	ns_vector_2i division_offset_in_image;
	unsigned long division_id;
	 bool blacked_out_non_subject_animals;

	ns_experiment_storyboard_timepoint * division;
	//returns true if a worm was picked and a change was made to the metadata
	const ns_experiment_storyboard_timepoint_element * get_worm_at_visualization_position(const ns_vector_2i & c) const {
		const ns_vector_2i p((c-division_offset_in_image)*resize_factor);
		//	ns_vector_2i rel_coords(image_coords*resize_factor);
		for (unsigned int i = 0; i < (*division).events.size(); i++){
			if (p.x >= (*division).events[i].position_on_time_point.x  &&

				p.y >= (*division).events[i].position_on_time_point.y &&

				p.x < (*division).events[i].position_on_time_point.x + (*division).events[i].image_image_size().x &&

				p.y < (*division).events[i].position_on_time_point.y + (*division).events[i].image_image_size().y
				)
					return &(*division).events[i];
		}
		return 0;
	}
	ns_experiment_storyboard_timepoint_element * get_worm_at_visualization_position(const ns_vector_2i & c) {
		const ns_vector_2i p((c-division_offset_in_image)*resize_factor);
		//	ns_vector_2i rel_coords(image_coords*resize_factor);
		for (unsigned int i = 0; i < (*division).events.size(); i++){
			if (p.x >= (*division).events[i].position_on_time_point.x  &&

				p.y >= (*division).events[i].position_on_time_point.y &&

				p.x < (*division).events[i].position_on_time_point.x + (*division).events[i].image_image_size().x &&

				p.y < (*division).events[i].position_on_time_point.y + (*division).events[i].image_image_size().y
				)
					return &(*division).events[i];
		}
		return 0;
	}

	void load_image(const unsigned long bottom_height,ns_annotater_image_buffer_entry & im,ns_sql & sql, ns_simple_local_image_cache & image_cache, ns_annotater_memory_pool & memory_pool, const unsigned long resize_factor_);
	std::vector<char> has_had_extra_worms_annotated;
};

class ns_worm_learner;
class ns_experiment_storyboard_annotater : public ns_image_series_annotater{
public:
	
	typedef enum {ns_show_all,ns_hide_censored,ns_hide_uncensored} ns_censor_masking;
	const ns_experiment_storyboard & get_storyboard(){return storyboard;}
	bool draw_group_ids;

	bool find_worm_by_id(const ns_64_bit region_id, const unsigned long group_id, ns_stationary_path_id & path_id, unsigned long & division_id, unsigned long & time) {
		for (unsigned int i = 0; i < divisions.size(); i++) {
			for (unsigned int j = 0; j < divisions[i].division->events.size(); j++) {
				//cout << i << " " << j << ": " << divisions[i].division->events[j].event_annotation.stationary_path_id.group_id << "\n";
				if ((region_id == 0 || divisions[i].division->events[j].event_annotation.region_info_id == region_id) &&
					divisions[i].division->events[j].event_annotation.stationary_path_id.group_id == group_id) {
					path_id = divisions[i].division->events[j].event_annotation.stationary_path_id;
					division_id = i;
					time = divisions[i].division->events[j].storyboard_absolute_time;
					return true;
				}
			}
		}
		//cout << "Could not find " << region_id << " " << group_id << "\n";
		return false;
	}


private:
	friend class ns_experiment_storyboard_annotater_timepoint;
	inline ns_annotater_timepoint * timepoint(const unsigned long i){
		return &divisions[i];
	}
	inline unsigned long number_of_timepoints(){
		return divisions.size();
	}

	

	std::vector<ns_experiment_storyboard_annotater_timepoint> divisions;
	ns_experiment_storyboard storyboard;
	
	static void draw_box(const ns_vector_2i & p, const ns_vector_2i & s, const ns_color_8 & c,ns_image_standard & im, const unsigned long thickness){
		im.draw_line_color_thick(p,p+ns_vector_2i(s.x,0),c,thickness);
		im.draw_line_color_thick(p,p+ns_vector_2i(0,s.y),c,thickness);
		im.draw_line_color_thick(p+s,p+ns_vector_2i(s.x,0),c,thickness);
		im.draw_line_color_thick(p+s,p+ns_vector_2i(0,s.y),c,thickness);
	}
	

	
	void draw_metadata(ns_annotater_timepoint * tp_a,ns_image_standard & im, double external_rescale_factor){
//		return;
	//	cerr << resize_factor << "\n";
		ns_experiment_storyboard_annotater_timepoint * tp(static_cast<ns_experiment_storyboard_annotater_timepoint * >(tp_a));
		const unsigned long thickness(3);
		const unsigned long thickness_2(1);
		const unsigned long thickness_offset(1);
		//ns_font & font(font_server.default_font());
		//unsigned long small_label_height(im.properties().height/80);
		//font.set_height(small_label_height);

		unsigned long uncensored_count(0),
					  censored_count(0);
		
		bool black_out_performed(false);
		for (unsigned int i = 0; i < tp->division->events.size(); i++){
				bool black_out = tp->division->events[i].annotation_was_censored_on_loading;
				//write over any animals that aren't of the specified strain
				if (!display_events_from_region.empty()){
					ns_event_display_spec_list::iterator p = display_events_from_region.find(tp->division->events[i].event_annotation.region_info_id);
					ns_death_time_annotation a(tp->division->events[i].event_annotation);
					if (p == display_events_from_region.end())
						throw ns_ex("Could not find region ") << tp->division->events[i].event_annotation.region_info_id << " in event strain specification";
					if (!p->second){
						black_out = true;
					}
				}
				if (black_out && !tp->blacked_out_non_subject_animals){
					black_out_performed = true;
					for (unsigned int y = 0; y < (tp->division->events[i].image_image_size()/resize_factor).y; y++){
						for (unsigned int x = 0; x < 3*(tp->division->events[i].image_image_size()/resize_factor).x; x++){
							unsigned int y_ = y+ tp->division_offset_in_image.y + tp->division->events[i].position_on_time_point.y/resize_factor,
										 x_ = x + 3*(tp->division_offset_in_image.x + tp->division->events[i].position_on_time_point.x/resize_factor);
								im[y_][x_] = .15*im[y_][x_];
						}
					}
				}
			
				if (!black_out){
					if (tp->division->events[i].event_annotation.is_excluded() || tp->division->events[i].event_annotation.flag.event_should_be_excluded()){
						censored_count++;
					}
					else uncensored_count++;
				}
		}
		if (black_out_performed)
				tp->blacked_out_non_subject_animals = true;
		ns_acquire_lock_for_scope lock(font_server.default_font_lock, __FILE__, __LINE__);
		ns_font & font(font_server.get_default_font());
		font.set_height(14);
		for (unsigned int i = 0; i < tp->division->events.size(); i++){
			//if (!tp->division->events[i].annotations_specifiy_censoring)
			//	continue;
			ns_color_8 color(ns_annotation_flag_color(tp->division->events[i].event_annotation));
			std::map<ns_64_bit,bool>::iterator p = excluded_regions.find(tp->division->events[i].event_annotation.region_info_id);
			if (p == excluded_regions.end())
				cerr << "Could not find reigon in excluded region list!\n";
			else{
				if (p->second &&
					!tp->division->events[i].event_annotation.is_excluded() &&
					!tp->division->events[i].event_annotation.flag.specified())
					color = ns_color_8(70,70,70);
			}
			//if (color = ns_color_8(0,0,0)
			//	continue;

			bool has_movement_events(false);
			for (unsigned int j = 0; j < tp->division->events[i].by_hand_movement_annotations_for_element.size(); j++){
				if (tp->division->events[i].by_hand_movement_annotations_for_element[j].annotation.type != ns_no_movement_event){
					has_movement_events = true;
					break;
				}
			}

			ns_vector_2i box_pos((tp->division_offset_in_image + tp->division->events[i].position_on_time_point/resize_factor)),
					box_size((tp->division->events[i].image_image_size()/resize_factor));

			if (!has_movement_events)
				im.draw_line_color_thick(box_pos,ns_vector_2i(box_pos.x+box_size.x/3,box_pos.y),ns_color_8(0,0,0),2);
			
		
			draw_box(box_pos,box_size,color,im,thickness);
			
			draw_box(box_pos,box_size, ns_color_8(0,0,0),im,thickness_2);

			if(!tp->division->events[i].drawn_worm_bottom_right_overlay){
				tp->division->events[i].drawn_worm_bottom_right_overlay = true;
				ns_color_8 br_tab_color;
				if (tp->division->events[i].event_annotation.type == ns_movement_cessation)
					br_tab_color = ns_color_8(255,0,0);
				else br_tab_color =  ns_color_8(0,255,0);
			
				unsigned long t_height(box_size.x/5);
				if (box_size.y/5 < t_height)
					t_height = box_size.y/5;

				ns_fill_bottom_flat_triangle(box_pos+box_size-ns_vector_2i(0,t_height),
											box_pos+box_size-ns_vector_2i(t_height,0),
											box_pos+box_size,
											br_tab_color,
											.25,
											im);
			}

			
					//im.draw_line_color(box_pos + ns_vector_2i((2*box_size.x)/3,0),ns_vector_2i(box_pos.x+box_size.x,box_pos.y),ns_color_8(200,200,255),2);
			if (has_movement_events & !tp->division->events[i].drawn_worm_top_right_overlay){
					unsigned long t_height(box_size.x/5);
					if (box_size.y/5 < t_height)
						t_height = box_size.y/5;
					ns_fill_top_flat_triangle(box_pos,
										box_pos+ns_vector_2i(t_height,0),
										box_pos+ns_vector_2i(0,t_height),
										ns_color_8(0,0,255),
										.25,
										im);
					tp->division->events[i].drawn_worm_top_right_overlay = true;
			}
			
			//we store whether or not extra worms have been annotated for the specified worm because we'll
			//need to write over the number afterwards.
			if (tp->has_had_extra_worms_annotated.size() != tp->division->events.size()){
				tp->has_had_extra_worms_annotated.resize(0);
				tp->has_had_extra_worms_annotated.resize(tp->division->events.size(),false);
			}

			if (draw_group_ids) {
				ns_vector_2i pos(box_pos);
				pos.y += box_size.y - 15;
				long box_offset(23);
				for (unsigned int y = 0; y < 15; y++)
					for (unsigned int x = 0; x < box_offset; x++) {
						im[pos.y + y][3 * (pos.x + x)] = 0;
						im[pos.y + y][3 * (pos.x + x) + 1] = 0;
						im[pos.y + y][3 * (pos.x + x) + 2] = 0;
					}	
				ns_font_output_dimension d(0, 0);
					font.draw_color(pos.x + 1, pos.y + 14, ns_color_8(255, 255, 255),
						ns_to_string(tp->division->events[i].event_annotation.stationary_path_id.group_id), im);
			}

			if (tp->division->events[i].event_annotation.number_of_worms_at_location_marked_by_hand> 0 ||
				tp->has_had_extra_worms_annotated[i]){
				ns_vector_2i pos(box_pos);
				long box_offset(13);
				pos.x+=box_size.x-box_offset;
				for (unsigned int y = 0; y < 15; y++)
					for (unsigned int x = 0; x < box_offset; x++){
						im[pos.y+y][3*(pos.x+x)] = 0;
						im[pos.y+y][3*(pos.x+x)+1] = 0;
						im[pos.y+y][3*(pos.x+x)+2] = 0;
					}
				ns_font_output_dimension d(0,0);
				if (tp->division->events[i].event_annotation.number_of_worms_at_location_marked_by_hand > 0){
					font.draw_color(pos.x+1+d.w,pos.y+14,ns_color_8(255,200,200),
						ns_to_string(tp->division->events[i].event_annotation.number_of_worms_at_location_marked_by_hand),im);
					tp->has_had_extra_worms_annotated[i] = true;
				}
				else tp->has_had_extra_worms_annotated[i] = false;
			}
			
		}
		
		font.set_height(bottom_text_size());
		ns_vector_2i label_position(im.properties().width/2,im.properties().height-bottom_text_offset_from_bottom());
		
		for (int y = ((int)im.properties().height)-bottom_border_height(); y <im.properties().height; y++)
			for (unsigned int x = 3*label_position.x; x < 3*im.properties().width; x++){
				im[y][x] = 0;
			}

		std::string count_text("Verified: ");
		count_text+=ns_to_string(uncensored_count);
		count_text+=" Excluded: ";
		count_text+=ns_to_string(censored_count);
		font.draw(label_position.x,label_position.y,ns_color_8(255,255,255),count_text,im);
		lock.release();
	}

	long side_border_width()const {return 15;}
	long bottom_border_height()const{return 25;}
	long bottom_text_offset_from_bottom()const{return 6;}
	long bottom_text_size()const{return (2*bottom_border_height())/3;}
	
	ns_image_properties division_image_properties;
	
	ns_experiment_storyboard_manager storyboard_manager;

	void initalize_division_image_population(ns_sql & sql){

		const unsigned long border(side_border_width()),
			bottom_border(bottom_border_height());

		ns_image_standard composit;
	
		if (!storyboard_manager.load_image_from_db(0,storyboard.subject(),composit,sql))
			throw ns_ex("Could not find storyboard image.");
		division_image_properties = composit.properties();
		division_image_properties.width = 0;
		division_image_properties.height = 0;
		for (unsigned int i = 0; i < divisions.size(); i++){
			if (divisions[i].division->size.x > division_image_properties.width)
				division_image_properties.width = divisions[i].division->size.x;
			if (divisions[i].division->size.y > division_image_properties.height)
				division_image_properties.height = divisions[i].division->size.y;
		}
		
		division_image_properties.height/=resize_factor;
		division_image_properties.width/=resize_factor;
		division_image_properties.height+=border + bottom_border;
		division_image_properties.width+=2*border;
		


	}
	void populate_division_images_from_composit(unsigned long division_image_id,ns_sql & sql){
		
		unsigned long label_position_y(division_image_properties.height-bottom_text_offset_from_bottom());

		const unsigned long border(side_border_width()),
			bottom_border(bottom_border_height());


		if (divisions[division_image_id].division_image.properties().width != 0)
			return;

		const unsigned long subimage_id(divisions[division_image_id].division->sub_image_id);

		ns_image_standard composit;
		if (!storyboard_manager.load_image_from_db(subimage_id,storyboard.subject(),composit,sql))
			throw ns_ex("Could not find storyboard image.");
		//cerr << "Composit height:" << composit.properties().height << "\n";
		bool use_color(composit.properties().components == 3);
		ns_acquire_lock_for_scope lock(font_server.default_font_lock, __FILE__, __LINE__);
		ns_font & font(font_server.get_default_font());
		font.set_height(bottom_text_size());

		for (unsigned int i = 0; i < divisions.size(); i++){
			if (divisions[i].division->sub_image_id != subimage_id)
				continue;
		//	cerr << "Division height: " << divisions[i].division->size.y << " / "  << division_image_properties.height << "\n";
			ns_vector_2i s(divisions[i].division->size/resize_factor);
			divisions[i].division_offset_in_image.x = border;
			divisions[i].division_offset_in_image.y = division_image_properties.height-bottom_border-s.y;

			if (resize_factor*s.y + divisions[i].division->position_on_storyboard.y >= composit.properties().height)
				throw ns_ex("Composite and division heights do not match!");
			if (resize_factor*s.x + divisions[i].division->position_on_storyboard.x >= composit.properties().width)
				throw ns_ex("Composite and division widths do not match!");

			divisions[i].division_image.prepare_to_recieve_image(division_image_properties);
			if (use_color){
				for (unsigned int y = 0; y < divisions[i].division_offset_in_image.y; y++)
					for (unsigned int x = 0; x < 3*division_image_properties.width; x++){
		//				if (y >= prop.height || x >= prop.width)
		//					throw ns_ex("YIKES!");
						divisions[i].division_image[y][x] = 0;
					}
			}
			else{
				
				for (unsigned int y = 0; y < divisions[i].division_offset_in_image.y; y++)
					for (unsigned int x = 0; x < division_image_properties.width; x++){
		//				if (y >= prop.height || x >= prop.width)
		//					throw ns_ex("YIKES!");
						divisions[i].division_image[y][x] = 0;
					}
			}
		
			if (use_color){
				for (unsigned int y = 0; y <s.y; y++){
					for (unsigned int x = 0; x < 3*division_image_properties.width; x++)
						divisions[i].division_image[divisions[i].division_offset_in_image.y+y][x] = 0;

					for (unsigned int x = 0; x < s.x; x++){
						for (unsigned int c = 0; c < 3; c++){
							divisions[i].division_image[(divisions[i].division_offset_in_image.y+y)]
														[(divisions[i].division_offset_in_image.x+x)*3+c] = 
							composit[resize_factor*y + divisions[i].division->position_on_storyboard.y]
									[(resize_factor*x + divisions[i].division->position_on_storyboard.x)*3+c];
						}
					}
				
					for (unsigned int x = 3*(s.x+divisions[i].division_offset_in_image.x); x < 3*division_image_properties.width; x++)
						divisions[i].division_image[divisions[i].division_offset_in_image.y+y][x] = 0;
				}
			}
			else{
				for (unsigned int y = 0; y <s.y; y++){
					for (unsigned int x = 0; x < division_image_properties.width; x++)
						divisions[i].division_image[divisions[i].division_offset_in_image.y+y][x] = 0;


					for (unsigned int x = 0; x < s.x; x++){
							divisions[i].division_image[divisions[i].division_offset_in_image.y+y]
														[divisions[i].division_offset_in_image.x+x] = 
							composit[resize_factor*y + divisions[i].division->position_on_storyboard.y]
									[resize_factor*x + divisions[i].division->position_on_storyboard.x];
					}
				
					for (unsigned int x = s.x+divisions[i].division_offset_in_image.x; x < division_image_properties.width; x++)
						divisions[i].division_image[divisions[i].division_offset_in_image.y+y][x] = 0;
				}

			}
			if (use_color){
				for (unsigned int y = s.y+divisions[i].division_offset_in_image.y; y < division_image_properties.height; y++)
					for (unsigned int x = 0; x < 3*division_image_properties.width; x++)
						divisions[i].division_image[y][x] = 0;
			}
			else{
				for (unsigned int y = s.y+divisions[i].division_offset_in_image.y; y < division_image_properties.height; y++)
					for (unsigned int x = 0; x < division_image_properties.width; x++)
						divisions[i].division_image[y][x] = 0;
			}

			std::string label;
			
			if (this->storyboard.subject().use_absolute_time)
				label+=(ns_format_time_string_for_human(divisions[i].division->time));
			else{ 
				label += "Day ";
				label += ns_to_string_short((divisions[i].division->time/(60.0*60*24)));
			}

				
			label += "          ";
			label += ns_to_string(i+1) + "/" + ns_to_string(divisions.size());
			if (use_color)
				font.draw_color(3*border,label_position_y,ns_color_8(255,255,255),label,divisions[i].division_image);
			else font.draw_grayscale(border,label_position_y,255,label,divisions[i].division_image);
		}
		lock.release();
	}

	ns_worm_learner * worm_learner;

	enum {default_resize_factor=2,max_buffer_size = 6};

	mutable std::set<ns_64_bit> regions_modified_but_not_saved;
	ns_region_metadata strain_to_display;
	ns_censor_masking censor_masking;
	typedef std::map<ns_64_bit,bool> ns_event_display_spec_list;
	ns_event_display_spec_list display_events_from_region;
	std::map<ns_64_bit,bool> excluded_regions;

	static ns_thread_return_type precache_worm_images_asynch_internal(void *);
	ns_sql * precache_sql_connection;
	bool run_precaching;
	ns_lock precaching_lock;
	ns_death_time_annotation_compiler machine_events_for_telemetry, all_events_for_telemetry;
public:

	ns_population_telemetry population_telemetry;
	void add_machine_events_for_telemetry(const ns_death_time_annotation_set& set,const ns_region_metadata & metadata) {
		machine_events_for_telemetry.add(set, metadata);
	}
	void clear_machine_events_for_telemetry() {
		machine_events_for_telemetry.clear();
	}
	void replot_telemetry() {
		population_telemetry.update_annotations_and_build_survival(all_events_for_telemetry, strain_to_display);
		draw_telemetry();
	}
	void rebuild_telemetry_with_by_hand_annotations () {
		std::map<ns_64_bit, ns_death_time_annotation_set> annotations;
		get_storyboard().collect_current_annotations(annotations);
		all_events_for_telemetry = machine_events_for_telemetry;
		for (std::map<ns_64_bit, ns_death_time_annotation_set>::const_iterator p = annotations.begin(); p != annotations.end(); p++)
			all_events_for_telemetry.add(p->second,ns_death_time_annotation_compiler::ns_do_not_create_regions_or_locations);

		population_telemetry.clear_cache();
		replot_telemetry();
	}
	ns_vector_2i telemetry_size(const ns_population_telemetry::ns_graph_contents& contents) const{
		if (contents == ns_population_telemetry::ns_survival)
			return ns_vector_2i(720, 400);
		else if (contents == ns_population_telemetry::ns_movement_vs_posture)
			return ns_vector_2i(720 + 480, 400);
		else throw ns_ex("Unknown population telemetry contents");
	}
	void draw_telemetry();
	void set_resize_factor(const unsigned long resize_factor_){
		resize_factor = resize_factor_;
	}
	bool data_needs_saving()const{return !regions_modified_but_not_saved.empty();}
	ns_experiment_storyboard_annotater(const unsigned long res):ns_image_series_annotater(res,0), draw_group_ids(0),precache_sql_connection(0),run_precaching(false), precaching_lock("pcl"){}
	~ns_experiment_storyboard_annotater() {
		run_precaching = false;
		precaching_lock.wait_to_acquire(__FILE__, __LINE__);
		precaching_lock.release();
		clear_precache_sql();
	}
	void clear_precache_sql() {
		ns_safe_delete(precache_sql_connection);
	}

	std::string image_label(const unsigned long frame_id){
		return ns_to_string(frame_id) + "/" + ns_to_string(divisions.size());	
	}

	bool load_annotations(){
		//annotations are loaded automatically when storyboard is loaded.
		return true;
	}
	void save_annotations(const ns_death_time_annotation_set & extra_annotations, std::set<ns_64_bit> & regions_modified) const{
		ns_acquire_for_scope<ns_sql> sql(image_server.new_sql_connection(__FILE__,__LINE__));
		storyboard.save_by_hand_annotations(sql(),extra_annotations);
		sql.release();
		ns_update_main_information_bar("Annotations saved at " + ns_format_time_string_for_human(ns_current_time()));
		ns_update_worm_information_bar("Annotations saved at " + ns_format_time_string_for_human(ns_current_time()));
		regions_modified = std::move(regions_modified_but_not_saved);
		regions_modified_but_not_saved.clear();
	};
	void redraw_current_metadata(double external_resize_factor){
		draw_metadata(&divisions[current_timepoint_id],*current_image.im, external_resize_factor);
	}
	void overlay_worm_ids() { draw_group_ids = true; }

	void precache_worm_images_asynch();
	void specifiy_worm_details(const ns_64_bit region_id,const ns_stationary_path_id & worm, const ns_death_time_annotation & sticky_properties, std::vector<ns_death_time_annotation> & movement_event_times){
		if (!worm.specified())
			throw ns_ex("ns_experiment_storyboard_annotater::specifiy_worm_details()::Requesting specification with unspecified path id");
		int found_count(0); 
		//search all divisions
		for (unsigned long j = 0; j < divisions.size(); j++){ 
			for (unsigned long i = 0; i < divisions[j].division->events.size(); i++){
				if (divisions[j].division->events[i].event_annotation.region_info_id == region_id &&
					divisions[j].division->events[i].event_annotation.stationary_path_id == worm){
					regions_modified_but_not_saved.insert(divisions[j].division->events[i].event_annotation.region_info_id);
					divisions[j].division->events[i].specify_by_hand_annotations(sticky_properties,movement_event_times);
					found_count++;
				}
			}
		}
		if (found_count == 0)
			throw ns_ex("ns_experiment_storyboard_annotater::specifiy_worm_details()::Could not find path: ") << worm.detection_set_id << ":" << worm.group_id << "," << worm.path_id; 

	}

	void load_from_storyboard(const ns_region_metadata & strain_to_display_, const ns_censor_masking censor_masking_, ns_experiment_storyboard_spec & spec, ns_worm_learner * worm_learner_, double external_rescale_factor);

	void register_click(const ns_vector_2i & image_position, const ns_click_request & action, double external_rescale_factor);

	void register_statistics_click(const ns_vector_2i& image_position, const ns_click_request& action, double external_rescale_factor);
	void load_random_worm();

	void display_current_frame();
	void stop_precaching() {
		run_precaching = false;
		precaching_lock.wait_to_acquire(__FILE__, __LINE__);
		precaching_lock.release();
	}
	void clear() {
		clear_base();
		regions_modified_but_not_saved.clear();
		divisions.resize(0);
		storyboard.clear();
		strain_to_display.clear();
		display_events_from_region.clear();
		clear_machine_events_for_telemetry();
		excluded_regions.clear();
	}

};
#endif