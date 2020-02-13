#ifndef NS_death_time_solo_posture_annotater_H
#define NS_death_time_solo_posture_annotater_H
#include "ns_image.h"
#include "ns_image_server.h"
#include "ns_image_series_annotater.h"
#include "ns_death_time_annotation.h"
#include "ns_time_path_image_analyzer.h"
#include "ns_death_time_posture_annotater.h"
#include "ns_hidden_markov_model_posture_analyzer.h"

#include "ns_fl_modal_dialogs.h"
void ns_hide_worm_window();

void ns_specifiy_worm_details(const unsigned long region_info_id,const ns_stationary_path_id & worm, const ns_death_time_annotation & sticky_properties, std::vector<ns_death_time_annotation> & event_times);

class ns_death_time_solo_posture_annotater_timepoint : public ns_annotater_timepoint{
private:
	//ns_movement_visualization_summary_entry summary_entry;

public:
	const ns_analyzed_image_time_path_element * path_timepoint_element;
	ns_time_path_image_movement_analyzer * movement_analyzer;
	unsigned long element_id;
	unsigned long group_id;
	//returns true if a worm was picked and a change was made to the metadata
	//const ns_movement_visualization_summary_entry * get_worm_at_visualization_position(const ns_vector_2i & c) const {
	//	return &summary_entry;
	//}
	//ns_movement_visualization_summary_entry * get_worm_at_visualization_position(const ns_vector_2i & c){
	//	return &summary_entry;
	//}
	enum {ns_movement_bar_height=15,ns_bottom_border_height_minus_hand_bars=115,ns_side_border_width=4, ns_minimum_width = 350};
	
	static int bottom_border_height(){return (int)ns_bottom_border_height_minus_hand_bars + ns_movement_bar_height*(int)ns_death_time_annotation::maximum_number_of_worms_at_position;}


	ns_image_storage_source_handle<ns_8_bit> get_image(ns_sql & sql){
		throw ns_ex("N/A");
	}
	void load_image(const unsigned long buffer_height,ns_annotater_image_buffer_entry & im,ns_sql & sql,ns_image_standard & temp_buffer,const unsigned long resize_factor_=1){
		movement_analyzer->load_images_for_group(group_id,element_id+10,sql,false);
	//	ns_annotater_timepoint::load_image(buffer_height,im,sql,temp_buffer,resize_factor_);
		if (!path_timepoint_element->registered_image_is_loaded()){
			cerr << "No image is loaded for current frame";
			return;
		}
		ns_image_properties prop(path_timepoint_element->image().properties());
		if (prop.width < ns_minimum_width)
			prop.width = ns_minimum_width;
		prop.width+=2*ns_side_border_width;
		prop.height+=bottom_border_height();
		prop.components = 3;
		im.loaded = true;
		im.im->init(prop);
		for (unsigned int y = 0; y < prop.height; y++)
			for (unsigned int x = 0; x < prop.width; x++){
				(*im.im)[y][3*x] = 
				(*im.im)[y][3*x+1] =
				(*im.im)[y][3*x+2] = 0;
			}
		
		for (unsigned int y = 0; y < path_timepoint_element->image().properties().height; y++)
			for (unsigned int x = 0; x < path_timepoint_element->image().properties().width; x++){
				(*im.im)[y][3*(x+ns_side_border_width)] = 
				(*im.im)[y][3*(x+ns_side_border_width)+1] =
				(*im.im)[y][3*(x+ns_side_border_width)+2] = path_timepoint_element->image()[y][x];

			}
		im.loaded = true;
	}
	
};

struct ns_animal_list_at_position{
	ns_stationary_path_id stationary_path_id;
	typedef std::vector<ns_death_timing_data> ns_animal_list;  //positions can hold multiple animals
	ns_animal_list animals;
};

class ns_death_time_posture_solo_annotater_region_data{
public:
	ns_death_time_posture_solo_annotater_region_data():movement_data_loaded(false),annotation_file("",""),loading_time(0),loaded_path_data_successfully(false){}
	bool loaded_path_data_successfully;
	void load_movement_analysis(const unsigned long region_id_,ns_sql & sql, const bool load_movement_quantification = false){
		solution.load_from_db(region_id_,sql,true);
		const ns_time_series_denoising_parameters time_series_denoising_parameters(ns_time_series_denoising_parameters::load_from_db(region_id_,sql));

		ns_acquire_for_scope<ns_analyzed_image_time_path_death_time_estimator> death_time_estimator(
				ns_get_death_time_estimator_from_posture_analysis_model(
				image_server.get_posture_analysis_model_for_region(region_id_,sql)));
		loaded_path_data_successfully = movement_analyzer.load_completed_analysis(region_id_,solution,time_series_denoising_parameters,&death_time_estimator(),sql,!load_movement_quantification);
		death_time_estimator.release();
	}
	void load_images_for_worm(const ns_stationary_path_id & path_id,const unsigned long number_of_images_to_load,ns_sql & sql){
		const unsigned long current_time(ns_current_time());
		const unsigned long max_number_of_cached_worms(2);
		//don't allow more than 2 paths to be loaded (to prevent memory overflow). Delete animals that have been in the cache for longest.
		if(image_loading_times_for_groups.size() >= max_number_of_cached_worms){
			
			//delete animals that are older than 5 minutes.
			const unsigned long cutoff_time(current_time-5*60);
			for (ns_loading_time_cache::iterator p = image_loading_times_for_groups.begin(); p != image_loading_times_for_groups.end(); ){
				if (p->second < cutoff_time){
					movement_analyzer.clear_images_for_group(p->first);
					image_loading_times_for_groups.erase(p++);
				}
				else p++;
			}
			//delete younger ones if necissary
			while (image_loading_times_for_groups.size() >= max_number_of_cached_worms){
				ns_loading_time_cache::iterator youngest(image_loading_times_for_groups.begin());
				for (ns_loading_time_cache::iterator p = image_loading_times_for_groups.begin(); p != image_loading_times_for_groups.end(); p++){
					if (youngest->second > p->second)
						youngest = p;
				}
				movement_analyzer.clear_images_for_group(youngest->first);
				image_loading_times_for_groups.erase(youngest);
			}
		}
		movement_analyzer.load_images_for_group(path_id.group_id,number_of_images_to_load,sql,false);
		image_loading_times_for_groups[path_id.group_id] = current_time;
	}	
	void clear_images_for_worm(const ns_stationary_path_id & path_id){
		movement_analyzer.clear_images_for_group(path_id.group_id);
		ns_loading_time_cache::iterator p = image_loading_times_for_groups.find(path_id.group_id);
		if (p != image_loading_times_for_groups.end())
			image_loading_times_for_groups.erase(p);
	}	

	ns_time_path_image_movement_analyzer movement_analyzer;

	std::vector<ns_animal_list_at_position> by_hand_timing_data; //organized by group.  that is movement_analyzer.group(4) is timing_data(4)
	std::vector<ns_animal_list_at_position> machine_timing_data; //organized by group.  that is movement_analyzer.group(4) is timing_data(4)
	std::vector<ns_death_time_annotation> orphaned_events;
	ns_region_metadata metadata;
	
	//group id,time of loading
	typedef std::map<long,unsigned long> ns_loading_time_cache;
	ns_loading_time_cache image_loading_times_for_groups;

	mutable ns_image_server_results_file annotation_file;
	unsigned long loading_time;
	bool load_annotations(ns_sql & sql, const bool load_by_hand=true){
		orphaned_events.resize(0);
		ns_timing_data_and_death_time_annotation_matcher <std::vector<ns_animal_list_at_position> > matcher;
		bool could_load_by_hand(false);
		if (load_by_hand){
			//load by hand annotations
			ns_acquire_for_scope<std::istream> in(annotation_file.input());
			if (!in.is_null()){
				ns_death_time_annotation_set set;
				set.read(ns_death_time_annotation_set::ns_all_annotations,in());
			
				std::string error_message;
				if (matcher.load_timing_data_from_set(set,false,by_hand_timing_data,orphaned_events,error_message)){
					if (error_message.size() != 0){
						ns_update_information_bar(error_message);
						could_load_by_hand = true;
					}

				}
			}
		}

		//load machine annotations
		ns_machine_analysis_data_loader machine_annotations;
		machine_annotations.load(ns_death_time_annotation_set::ns_censoring_and_movement_transitions,metadata.region_id,0,0,sql,true);
		if (machine_annotations.samples.size() != 0 && machine_annotations.samples.begin()->regions.size() != 0){
			std::vector<ns_death_time_annotation> _unused_orphaned_events;
			std::string _unused_error_message;
			matcher.load_timing_data_from_set((*machine_annotations.samples.begin()->regions.begin())->death_time_annotation_set,true,
				machine_timing_data,_unused_orphaned_events,_unused_error_message);
			
			return true;
		}

		
	
		return could_load_by_hand;
	}
	void add_annotations_to_set(ns_death_time_annotation_set & set, std::vector<ns_death_time_annotation> & orphaned_events){
		ns_death_time_annotation_set t_set;
		std::vector<ns_death_time_annotation> t_orphaned_events;
		ns_timing_data_and_death_time_annotation_matcher<std::vector<ns_death_timing_data> > matcher;
		for (unsigned int i = 0; i < by_hand_timing_data.size(); i++)
			matcher.save_death_timing_data_to_set(by_hand_timing_data[i].animals,t_orphaned_events,t_set, false);
		orphaned_events.insert(orphaned_events.end(),t_orphaned_events.begin(),t_orphaned_events.end());
		set.add(t_set);
	}

	void save_annotations(const ns_death_time_annotation_set & extra_annotations) const{
		throw ns_ex("The solo posture annotations should be mixed in with storyboard annotations!");
		ns_death_time_annotation_set set;
		

		ns_acquire_for_scope<std::ostream> out(annotation_file.output());
		if (out.is_null())
			throw ns_ex("Could not open output file.");
		set.write(out());
		out.release();
	//	ns_update_information_bar(ns_to_string(set.events.size()) + " events saved at " + ns_format_time_string_for_human(ns_current_time()));
	//	saved_=true;
	};
private:
	ns_time_path_solution solution;
	bool movement_data_loaded;
};




class ns_death_time_posture_solo_annotater_data_cache{
	
	//region_info_id , data
	typedef std::map<unsigned long,ns_death_time_posture_solo_annotater_region_data> ns_movement_data_list;
	ns_movement_data_list region_movement_data;

public:
	void clear(){
		region_movement_data.clear();
	}
	ns_death_time_posture_solo_annotater_region_data * get_region_movement_data(const unsigned int region_id,ns_sql & sql){
		ns_movement_data_list::iterator p = region_movement_data.find(region_id);
		if (p == region_movement_data.end()){
			unsigned long current_time(ns_current_time());
			//clear out old cache entries to prevent large cumulative memory allocation
			unsigned long cleared(0);
			unsigned long count(region_movement_data.size());
			unsigned long cutoff_time;
			unsigned long max_buffer_size(3);
			if (region_movement_data.size() < max_buffer_size){
				//delete all cached data older than 3 minutes
				cutoff_time = current_time-60*60*3;
			}
			else{
				std::vector<unsigned long> times(region_movement_data.size());
				unsigned long i(0);
				for (ns_movement_data_list::iterator q = region_movement_data.begin();  q != region_movement_data.end();q++){
					times[i] = q->second.loading_time;
					i++;
				}
				std::sort(times.begin(),times.end(), std::greater<int>());
				cutoff_time = times[max_buffer_size-1];
			}
			for (ns_movement_data_list::iterator q = region_movement_data.begin();  q != region_movement_data.end();){
				if (q->second.loading_time <= cutoff_time){
					region_movement_data.erase(q++);
					cleared++;
				}
				else q++;
			}
			if (cleared > 0)
				cout << "Cleared " << cleared << " out of " << count << " entries in path cache.\n";
			p = region_movement_data.insert(ns_movement_data_list::value_type(region_id,ns_death_time_posture_solo_annotater_region_data())).first;
			try{
				p->second.metadata.load_from_db(region_id,"",sql);
				ns_image_server_results_subject sub;
				sub.region_id = region_id;
				p->second.loading_time = current_time;
				p->second.annotation_file = image_server.results_storage.hand_curated_death_times(sub,sql);
				//this will work even if no path data can be loaded, but 
				//p->loaded_path_data_successfully will be set to false
				p->second.load_movement_analysis(region_id,sql);
				
				p->second.by_hand_timing_data.resize(p->second.movement_analyzer.size());
				p->second.machine_timing_data.resize(p->second.movement_analyzer.size());
				for (unsigned int i = 0; i < p->second.movement_analyzer.size(); i++){
					ns_stationary_path_id path_id;
					path_id.detection_set_id = p->second.movement_analyzer.db_analysis_id();
					path_id.group_id = i;
					path_id.path_id = 0;
					p->second.by_hand_timing_data[i].stationary_path_id = path_id;
					p->second.machine_timing_data[i].stationary_path_id = path_id;

					p->second.by_hand_timing_data[i].animals.resize(1);
					p->second.by_hand_timing_data[i].animals[0].set_fast_movement_cessation_time(
						ns_death_timing_data_step_event_specification(
						p->second.movement_analyzer[i].paths[0].cessation_of_fast_movement_interval(),
						p->second.movement_analyzer[i].paths[0].element(p->second.movement_analyzer[i].paths[0].first_stationary_timepoint()),
											region_id,path_id,0));
					p->second.by_hand_timing_data[i].animals[0].animal_specific_sticky_properties.animal_id_at_position = 0;
					p->second.machine_timing_data[i].animals.resize(1);
					p->second.machine_timing_data[i].animals[0].set_fast_movement_cessation_time(
						ns_death_timing_data_step_event_specification(
						p->second.movement_analyzer[i].paths[0].cessation_of_fast_movement_interval(),
						p->second.movement_analyzer[i].paths[0].element(p->second.movement_analyzer[i].paths[0].first_stationary_timepoint()),
											region_id,path_id,0));
					p->second.machine_timing_data[i].animals[0].animal_specific_sticky_properties.animal_id_at_position = 0;
					p->second.by_hand_timing_data[i].animals[0].position_data.stationary_path_id = path_id;
					p->second.by_hand_timing_data[i].animals[0].position_data.path_in_source_image.position = p->second.movement_analyzer[i].paths[0].path_region_position;
					p->second.by_hand_timing_data[i].animals[0].position_data.path_in_source_image.size = p->second.movement_analyzer[i].paths[0].path_region_size;
					p->second.by_hand_timing_data[i].animals[0].position_data.worm_in_source_image.position = p->second.movement_analyzer[i].paths[0].element(p->second.movement_analyzer[i].paths[0].first_stationary_timepoint()).region_offset_in_source_image();
					p->second.by_hand_timing_data[i].animals[0].position_data.worm_in_source_image.size = p->second.movement_analyzer[i].paths[0].element(p->second.movement_analyzer[i].paths[0].first_stationary_timepoint()).worm_region_size();
				
					p->second.by_hand_timing_data[i].animals[0].animal_specific_sticky_properties.stationary_path_id = p->second.by_hand_timing_data[i].animals[0].position_data.stationary_path_id;
					//p->second.by_hand_timing_data[i].animals[0].worm_id_in_path = 0;
						//p->second.by_hand_timing_data[i].sticky_properties.annotation_source = ns_death_time_annotation::ns_lifespan_machine;
					//p->second.by_hand_timing_data[i].sticky_properties.position = p->second.movement_analyzer[i].paths[0].path_region_position;
				//	p->second.by_hand_timing_data[i].sticky_properties.size = p->second.movement_analyzer[i].paths[0].path_region_size;

				//	p->second.by_hand_timing_data[i].stationary_path_id = p->second.by_hand_timing_data[i].position_data.stationary_path_id;

					p->second.by_hand_timing_data[i].animals[0].region_info_id = region_id;
					p->second.machine_timing_data[i].animals[0] = 
						p->second.by_hand_timing_data[i].animals[0];

					//by default specify the beginnnig of the path as the translation cessation time.
					p->second.by_hand_timing_data[i].animals[0].step_event(
						ns_death_timing_data_step_event_specification(
									p->second.movement_analyzer[i].paths[0].cessation_of_fast_movement_interval(),
									p->second.movement_analyzer[i].paths[0].element(p->second.movement_analyzer[i].paths[0].first_stationary_timepoint()),region_id,
									p->second.by_hand_timing_data[i].animals[0].position_data.stationary_path_id,0),p->second.movement_analyzer[i].paths[0].observation_limits());
				}
				p->second.load_annotations(sql,false);
			}
			catch(...){
				region_movement_data.erase(p);
				throw;
			}
		}
		return &p->second;
	}


	void add_annotations_to_set(ns_death_time_annotation_set & set, std::vector<ns_death_time_annotation> & orphaned_events){
		for (ns_death_time_posture_solo_annotater_data_cache::ns_movement_data_list::iterator p = region_movement_data.begin(); p != region_movement_data.end(); p++)
			p->second.add_annotations_to_set(set,orphaned_events);
	}
	bool save_annotations(const ns_death_time_annotation_set & a){
		throw ns_ex("Solo posture annotater annotations should be mixed in with storyboard");
		for (ns_death_time_posture_solo_annotater_data_cache::ns_movement_data_list::iterator p = region_movement_data.begin(); p != region_movement_data.end(); p++)
			p->second.save_annotations(a);
		return true;
	}
};
void ns_crop_time(const ns_time_path_limits & limits, const ns_death_time_annotation_time_interval & first_observation_in_path, const ns_death_time_annotation_time_interval & last_observation_in_path, ns_death_time_annotation_time_interval & target);

class ns_worm_learner;
class ns_death_time_solo_posture_annotater : public ns_image_series_annotater{
private:

	std::vector<ns_death_time_solo_posture_annotater_timepoint> timepoints;
	static ns_death_time_posture_solo_annotater_data_cache data_cache;
	ns_death_time_posture_solo_annotater_region_data * current_region_data;
	ns_analyzed_image_time_path * current_worm;
	ns_death_time_annotation properties_for_all_animals;

	unsigned long current_element_id()const {return current_timepoint_id;}
	ns_death_time_annotation_time_interval current_time_interval(){
		if (current_timepoint_id == 0)
			return current_worm->observation_limits().interval_before_first_observation;
		return ns_death_time_annotation_time_interval(current_worm->element(current_timepoint_id-1).absolute_time,
													  current_worm->element(current_timepoint_id).absolute_time);

	}
	ns_animal_list_at_position & current_by_hand_timing_data(){return current_region_data->by_hand_timing_data[properties_for_all_animals.stationary_path_id.group_id];}
	ns_animal_list_at_position * current_machine_timing_data;
	unsigned long current_animal_id;
	
	static void draw_box(const ns_vector_2i & p, const ns_vector_2i & s, const ns_color_8 & c,ns_image_standard & im, const unsigned long thickness){
		im.draw_line_color(p,p+ns_vector_2i(s.x,0),c,thickness);
		im.draw_line_color(p,p+ns_vector_2i(0,s.y),c,thickness);
		im.draw_line_color(p+s,p+ns_vector_2i(s.x,0),c,thickness);
		im.draw_line_color(p+s,p+ns_vector_2i(0,s.y),c,thickness);
	}
	static std::string state_label(const ns_movement_state cur_state){
		switch(cur_state){
			case ns_movement_stationary:return "Dead";
			case ns_movement_posture: return "Changing Posture";
			case ns_movement_slow: return "Slow Moving";
			case ns_movement_fast: return "Fast Moving";
			case ns_movement_death_posture_relaxation: return "Death Posture Relaxation";
			default: return std::string("Unknown:") + ns_to_string((int)cur_state);
		}
	}
	enum{bottom_offset=5};
	ns_vector_2i bottom_margin_position(){
		if (!current_worm->element(current_element_id()).registered_image_is_loaded()){
			cerr << "No image is loaded for current element;";
			return ns_vector_2i(0,0);
		}
		return ns_vector_2i(ns_death_time_solo_posture_annotater_timepoint::ns_side_border_width,
		current_worm->element(current_element_id()).image().properties().height+bottom_offset);}

	void draw_metadata(ns_annotater_timepoint * tp_a,ns_image_standard & im){
	//	cerr << resize_factor << "\n";	
		//no image!  
		if (im.properties().height == 0 || im.properties().width == 0)
			throw ns_ex("No worm image is loaded");
		const ns_death_time_solo_posture_annotater_timepoint * tp(static_cast<const ns_death_time_solo_posture_annotater_timepoint * >(tp_a));
		const unsigned long clear_thickness(5);
		const unsigned long thickness_offset(3);
		ns_font & font(font_server.default_font());
		const unsigned long text_height(14);
		font.set_height(text_height);
		ns_vector_2i bottom_margin_bottom(bottom_margin_position());
		ns_vector_2i bottom_margin_top(bottom_margin_bottom.x+current_worm->element(current_element_id()).image().properties().width + ns_death_time_solo_posture_annotater_timepoint::ns_side_border_width,
										bottom_margin_bottom.y + ns_death_time_solo_posture_annotater_timepoint::bottom_border_height()-bottom_offset);
	
		for (unsigned int y = bottom_margin_bottom.y; y < bottom_margin_top.y; y++)
			for (unsigned int x = bottom_margin_bottom.x; x < bottom_margin_top.x; x++){
				im[y][3*x] = 
				im[y][3*x+1] = 
				im[y][3*x+2] = 0;
			}
				
		
		unsigned long vis_height(10);
		const ns_time_path_limits observation_limit(current_worm->observation_limits());
		ns_death_time_annotation_time_interval first_path_obs,last_path_obs;
		last_path_obs.period_end = current_worm->element(current_worm->element_count()-1).absolute_time;
		last_path_obs.period_start = current_worm->element(current_worm->element_count()-2).absolute_time;
		first_path_obs.period_start = current_worm->element(0).absolute_time;
		first_path_obs.period_end = current_worm->element(1).absolute_time;

		//handle out of bound values
		for (ns_animal_list_at_position::ns_animal_list::iterator p = current_by_hand_timing_data().animals.begin(); p != current_by_hand_timing_data().animals.end(); p++){
			ns_crop_time(observation_limit,first_path_obs,last_path_obs,p->fast_movement_cessation.time);
			ns_crop_time(observation_limit,first_path_obs,last_path_obs,p->death_posture_relaxation_termination.time);
			ns_crop_time(observation_limit,first_path_obs,last_path_obs,p->movement_cessation.time);
			ns_crop_time(observation_limit,first_path_obs,last_path_obs,p->translation_cessation.time);
		}

		current_machine_timing_data->animals[0].draw_movement_diagram(bottom_margin_bottom,
												ns_vector_2i(current_worm->element(current_element_id()).image().properties().width,vis_height),
												observation_limit,
												current_time_interval(),
												im,0.8);
		const unsigned long hand_bar_height(current_by_hand_timing_data().animals.size()*
			ns_death_time_solo_posture_annotater_timepoint::ns_movement_bar_height);
		
		for (unsigned int i = 0; i < current_by_hand_timing_data().animals.size(); i++){
			current_by_hand_timing_data().animals[i].draw_movement_diagram(bottom_margin_bottom
				+ns_vector_2i(0,(i+1)*ns_death_time_solo_posture_annotater_timepoint::ns_movement_bar_height),
													ns_vector_2i(current_worm->element(current_element_id()).image().properties().width,vis_height),
													observation_limit,
													current_time_interval(),
													im,(i==current_animal_id)?1.0:0.8);
		}
		
		const int num_lines(6);
		std::string lines[num_lines];
		ns_color_8 line_color[num_lines];
		ns_movement_state cur_by_hand_state(current_by_hand_timing_data().animals[current_animal_id].movement_state(current_worm->element(current_element_id()).absolute_time));
		ns_movement_state cur_machine_state(current_machine_timing_data->animals[0].movement_state(current_worm->element(current_element_id()).absolute_time));
		lines[0]  = "Frame " + ns_to_string(current_element_id()+1) + " of " + ns_to_string(timepoints.size())+ " ";
		lines[0] += current_worm->element(current_element_id()).element_before_fast_movement_cessation?"(B)":"(A)";
		lines[0] += current_worm->element(current_element_id()).inferred_animal_location?"(I)":"";
		if (current_region_data->metadata.time_at_which_animals_had_zero_age != 0){
			lines[0] += "Day ";
			lines[0] += ns_to_string_short((float)
				(current_worm->element(current_element_id()).absolute_time-
				current_region_data->metadata.time_at_which_animals_had_zero_age )/24.0/60/60,2);
			lines[0] += " ";
		}
		lines[0]+= "Date: ";
		lines[0]+= ns_format_time_string_for_human(current_worm->element(current_element_id()).absolute_time);
		
		lines[1] = (current_region_data->metadata.plate_type_summary());

	
		lines[2] = current_region_data->metadata.sample_name + "::" + current_region_data->metadata.region_name + "::worm #" + ns_to_string(properties_for_all_animals.stationary_path_id.group_id);

		ns_vector_2i p(current_worm->path_region_position + current_worm->path_region_size/2);
		lines[2] += " (" + ns_to_string(p.x) + "," + ns_to_string(p.y) + ")";

		lines[3] = ("Machine:" + state_label(cur_machine_state));
		if (current_by_hand_timing_data().animals[current_animal_id].specified)
			lines[4] += "Human: " + state_label(cur_by_hand_state);
		else lines[4] += "Human: Not Specified";
		if (properties_for_all_animals.number_of_worms_at_location_marked_by_hand > 1){
			lines [4] += " +";
			lines[4] += ns_to_string(properties_for_all_animals.number_of_worms_at_location_marked_by_hand);
		}

		if (properties_for_all_animals.is_excluded())
			lines[5] = "Worm Excluded ";
		if (properties_for_all_animals.flag.specified())
			lines[5] += properties_for_all_animals.flag.label();
		

		line_color[0] = line_color[1] = line_color[2] = line_color[3] = line_color[4] = 
			 ns_color_8(255,255,255);
		line_color[5] = ns_annotation_flag_color(properties_for_all_animals);

		ns_vector_2i text_pos(bottom_margin_bottom.x,bottom_margin_bottom.y+hand_bar_height+15+text_height);
		for (unsigned int i = 0; i < num_lines; i++)
			font.draw_color(text_pos.x,text_pos.y+i*(text_height+1),line_color[i],lines[i],im);
		
		//unsigned long small_label_height(im.properties().height/80);
		//font.set_height(small_label_height);

		//clear out old box
		ns_vector_2d off(ns_analyzed_image_time_path::maximum_alignment_offset() + current_worm->element(current_element_id()).offset_in_registered_image());
		draw_box(ns_vector_2i(off.x+ns_death_time_solo_posture_annotater_timepoint::ns_side_border_width,off.y),
			current_worm->element(current_element_id()).worm_context_size(),
				  ns_color_8(60,60,60),im,2);

		draw_box(ns_vector_2i(ns_death_time_solo_posture_annotater_timepoint::ns_side_border_width,0),
				 ns_vector_2i(timepoints[current_element_id()].path_timepoint_element->image().properties().width, 
				 timepoints[current_element_id()].path_timepoint_element->image().properties().height), 
				ns_color_8(60,60,60),im,2);
	
		draw_box(ns_vector_2i(ns_death_time_solo_posture_annotater_timepoint::ns_side_border_width,0),
				 ns_vector_2i(timepoints[current_element_id()].path_timepoint_element->image().properties().width, 
				 timepoints[current_element_id()].path_timepoint_element->image().properties().height), 
				ns_color_8(60,60,60),im,2);

	//	draw_box(timepoints[current_element_id()].path_timepoint_element->measurements.local_maximum_position, 
	//			 timepoints[current_element_id()].path_timepoint_element->measurements.local_maximum_area,
	//			ns_color_8(0,0,60),im,1);

//		font.set_height(im.properties().height/40);
	//	font.draw_color(1,im.properties().height/39,ns_color_8(255,255,255),ns_format_time_string_for_human(tp->absolute_time),im);
	}

	

	ns_worm_learner * worm_learner;


	enum {default_resize_factor=1,max_buffer_size = 15};

	mutable bool saved_;
public:
	
	typedef enum {ns_none,ns_forward, ns_back, ns_fast_forward, ns_fast_back,ns_stop,ns_save,ns_rewind_to_zero,ns_write_quantification_to_disk,ns_number_of_annotater_actions} ns_image_series_annotater_action;

	void clear_cache(){
		close_worm();
		data_cache.clear();
		current_region_data = 0;
		properties_for_all_animals = ns_death_time_annotation();
	}
/* // output_movement_quantification is never called anywhere...
	void output_movement_quantification(){
		throw ns_ex("DDData not loaded");
		string filename(save_file_dialog("Save Movement Quantification",vector<dialog_file_type>(1,dialog_file_type("CSV","csv")),"csv","movement_quantification.csv"));
		if (filename == "")
			return;
		ofstream o(filename.c_str());
		if (o.fail())
			throw ns_ex("Could not write to file ") << filename;
		current_worm->write_detailed_movement_quantification_analysis_header(o);
		current_worm->write_detailed_movement_quantification_analysis_data(current_region_data->metadata,properties_for_all_animals.stationary_path_id.group_id,properties_for_all_animals.stationary_path_id.path_id,o,false);
		o.close();
	}/*

	/*bool step_forward(bool asynch=false){
		const bool ret(ns_image_series_annotater::step_forward(asynch));
		if (ret)
			current_element_id++;
		return ret;
	}
	bool step_back(bool asynch=false){	
		const bool ret(ns_image_series_annotater::step_back(asynch));
		if(ret)
			current_element_id--;	
		return ret;
	}*/

	inline ns_annotater_timepoint * timepoint(const unsigned long i){return &timepoints[i];}
	inline unsigned long number_of_timepoints(){return timepoints.size();}
	
	void set_resize_factor(const unsigned long resize_factor_){resize_factor = resize_factor_;}
	bool data_saved()const{return saved_;}
	ns_death_time_solo_posture_annotater():ns_image_series_annotater(default_resize_factor, ns_death_time_posture_annotater_timepoint::ns_bottom_border_height),
		saved_(true),current_region_data(0),current_worm(0),current_machine_timing_data(0){}

	typedef enum {ns_time_aligned_images,ns_death_aligned_images} ns_alignment_type;

	bool load_annotations(){
	//	for (u
		saved_=true;
	//	saved_ = !fix_censored_events_with_no_time_specified();
		return true;
	}

	void add_annotations_to_set(ns_death_time_annotation_set & set, std::vector<ns_death_time_annotation> & orphaned_events){
		data_cache.add_annotations_to_set(set,orphaned_events);
	}

	void save_annotations(const ns_death_time_annotation_set & set) const{
		data_cache.save_annotations(set);
	
		ns_update_information_bar(string("Annotations saved at ") + ns_format_time_string_for_human(ns_current_time()));
		saved_=true;
	};
	bool current_region_path_data_loaded(){
		return current_region_data->loaded_path_data_successfully;
	}
	void close_worm(){
		properties_for_all_animals = ns_death_time_annotation();
		properties_for_all_animals.region_info_id = 0;
		current_worm = 0;
		current_machine_timing_data = 0;
		current_timepoint_id = 0;
		current_animal_id = 0;
		if (!sql.is_null())
			sql.release();
	}

	static bool ns_fix_annotation(ns_death_time_annotation & a,ns_analyzed_image_time_path & p);
	void set_current_timepoint(const unsigned long current_time,const bool require_exact=true,const bool give_up_if_too_long=false){
		for (current_timepoint_id = 0; current_timepoint_id < current_worm->element_count(); current_timepoint_id++){
				if (current_worm->element(current_timepoint_id).absolute_time >= current_time)
					break;
			}	
		bool give_up_on_preload(false);
		if (current_timepoint_id > 50 && give_up_if_too_long){
			give_up_on_preload = true;
			current_timepoint_id = 0;
			for (unsigned int k = 0; k < current_worm->element_count(); k++)
				if (!current_worm->element(k).element_before_fast_movement_cessation){
					current_timepoint_id = k;
					break;
				}
		}

		//load images for the worm.
			
		//data_cache.load_images_for_worm(properties_for_all_animals.stationary_path_id,current_timepoint_id+10,sql);
		current_region_data->clear_images_for_worm(properties_for_all_animals.stationary_path_id);

		current_region_data->load_images_for_worm(properties_for_all_animals.stationary_path_id,current_timepoint_id+10,sql());


		if (require_exact && current_worm->element(current_timepoint_id).absolute_time != current_time && !give_up_on_preload)
			throw ns_ex("Could not find requested element time:") << current_time;
		if (current_timepoint_id+1 < current_worm->element_count())
			current_timepoint_id++;

	}

	void load_worm(const unsigned long region_info_id_, const ns_stationary_path_id & worm, const unsigned long current_time,const ns_experiment_storyboard  * storyboard,ns_worm_learner * worm_learner_,ns_sql & sql){
		stop_fast_movement();
		clear();
		ns_image_series_annotater::clear();
		this->close_worm();
		ns_region_metadata metadata;
		metadata.region_id = region_info_id_;
		try{
			const ns_experiment_storyboard_timepoint_element & e(storyboard->find_animal(region_info_id_,worm));
			ns_acquire_lock_for_scope lock(image_buffer_access_lock,__FILE__,__LINE__);
			worm_learner = worm_learner_;
		
			timepoints.resize(0);
			if (current_region_data != 0)
				current_region_data->clear_images_for_worm(properties_for_all_animals.stationary_path_id);
	
			properties_for_all_animals.region_info_id = region_info_id_;
			//get movement information for current worm
			try{
				current_region_data = data_cache.get_region_movement_data(region_info_id_,sql);
			}
			catch(...){
				metadata.load_from_db(region_info_id_,"",sql);
				throw;
			}
			if (!current_region_data->loaded_path_data_successfully)
					throw ns_ex("The movement anaylsis data required to inspect this worm is no longer in the database.  You might need to re-analyze movement for this region.");
			metadata = current_region_data->metadata;
			if (worm.detection_set_id != 0 && current_region_data->movement_analyzer.db_analysis_id() != worm.detection_set_id)
				throw ns_ex("This storyboard was built using an out-of-date movement analysis result.  Please rebuild the storyboard, so that it will reflect the most recent analysis.");
			if (current_region_data->movement_analyzer.size() <= worm.group_id)
				throw ns_ex("Looking at the movement analysis result for this region, this worm's ID appears to be invalid.");
			if (worm.path_id != 0)
				throw ns_ex("The specified group has multiple paths! This should be impossible");
			current_worm = &current_region_data->movement_analyzer[worm.group_id].paths[worm.path_id];
			current_animal_id = 0;

		//	if (current_region_data->by_hand_timing_data[worm.group_id].animals.size() == 0)
		//		cerr << "WHA";
			for (unsigned int i = 0; i < e.by_hand_movement_annotations_for_element.size(); i++){
				unsigned long animal_id = e.by_hand_movement_annotations_for_element[i].annotation.animal_id_at_position;
				const unsigned long current_number_of_animals(current_region_data->by_hand_timing_data[worm.group_id].animals.size());
				if (animal_id >= current_number_of_animals){
					current_region_data->by_hand_timing_data[worm.group_id].animals.resize(animal_id+1);
					for (unsigned int i = current_number_of_animals; i < animal_id; i++){
						current_region_data->by_hand_timing_data[worm.group_id].animals[i].set_fast_movement_cessation_time(
							ns_death_timing_data_step_event_specification(current_worm->cessation_of_fast_movement_interval(),
							current_worm->element(current_worm->first_stationary_timepoint()),
							properties_for_all_animals.region_info_id,properties_for_all_animals.stationary_path_id,i));
					}
					
				}
				current_region_data->by_hand_timing_data[worm.group_id].animals[animal_id].add_annotation(e.by_hand_movement_annotations_for_element[i].annotation,true);
				//if (animal_id == 0)
				//if (e.by_hand_movement_annotations_for_element[i].type == ns_additional_worm_entry)
				//	curre nt_region_data->by_hand_timing_data[worm.group_id].animals[animal_id].first_frame_time = e.by_hand_movement_annotations_for_element[i].time.end_time;
				//current_region_data->by_hand_timing_data[worm.group_id].animals[animal_id].add_annotation(e.by_hand_movement_annotations_for_element[i],true);
			}
			//current_region_data->metadata
			//current_by_hand_timing_data = &current_region_data->by_hand_timing_data[worm.group_id];
			if (worm.group_id >= current_region_data->machine_timing_data.size())
				throw ns_ex("Group ID ") << worm.group_id << " is not present in machine timing data (" << current_region_data->machine_timing_data.size() << ") for region " << region_info_id_;
			current_machine_timing_data = &current_region_data->machine_timing_data[worm.group_id];
			current_animal_id = 0;
			//current_by_hand_timing_data->animals[current_animal_id].sticky_properties = current_machine_timing_data->animals[0].sticky_properties;
			e.event_annotation.transfer_sticky_properties(properties_for_all_animals);
			//current_by_hand_timing_data->animals[current_animal_id].sticky_properties.annotation_source = ns_death_time_annotation::ns_posture_image;
			properties_for_all_animals.stationary_path_id = worm;
			this->saved_ = true;
			for (unsigned int j = 0; j < current_region_data->by_hand_timing_data[worm.group_id].animals.size(); j++){
				if (ns_fix_annotation(current_region_data->by_hand_timing_data[worm.group_id].animals[j].fast_movement_cessation,*current_worm))
					saved_ = false;
				if (ns_fix_annotation(current_region_data->by_hand_timing_data[worm.group_id].animals[j].translation_cessation,*current_worm))
					saved_ = false;
				if (ns_fix_annotation(current_region_data->by_hand_timing_data[worm.group_id].animals[j].movement_cessation,*current_worm))
					saved_ = false;
				if (ns_fix_annotation(current_region_data->by_hand_timing_data[worm.group_id].animals[j].death_posture_relaxation_termination,*current_worm))
					saved_ = false;
			}
			if (!saved_){
				update_events_to_storyboard();
				saved_ = true;
			}
			//initialize worm timing data
			unsigned long number_of_valid_elements;
			for ( number_of_valid_elements = 0;  number_of_valid_elements < current_worm->element_count();  number_of_valid_elements++)
				if ( current_worm->element( number_of_valid_elements).excluded)
					break;
		
			timepoints.resize(number_of_valid_elements);
			if (number_of_valid_elements == 0)
				throw ns_ex("This path has no valid elements!");
			for (unsigned int i = 0; i < timepoints.size(); i++){
				timepoints[i].path_timepoint_element = &current_worm->element(i);
				timepoints[i].element_id = i;
				timepoints[i].movement_analyzer = & current_region_data->movement_analyzer;
				timepoints[i].group_id = properties_for_all_animals.stationary_path_id.group_id;
			}
//			current_by_hand_timing_data->animals[current_animal_id].first_frame_time = timepoints[0].path_timepoint_element->absolute_time;


			//allocate image buffer
			if (previous_images.size() != max_buffer_size || next_images.size() != max_buffer_size){
				previous_images.resize(max_buffer_size);
				next_images.resize(max_buffer_size);
				for (unsigned int i = 0; i < max_buffer_size; i++){
					previous_images[i].im = new ns_image_standard();
					next_images[i].im = new ns_image_standard();
				}
			}
			if (current_image.im == 0)
				current_image.im = new ns_image_standard();

			this->sql.attach(&sql);

			set_current_timepoint(current_time,true,true);
			{
				ns_image_standard temp_buffer;
				timepoints[current_timepoint_id].load_image(1024,current_image,sql,temp_buffer,1);
			}
			draw_metadata(&timepoints[current_timepoint_id],*current_image.im);
			
			request_refresh();
			lock.release();

		}
		catch(ns_ex & ex){
			close_worm();
			cerr << "Error loading worm from region " << metadata.plate_name() << " : " << ex.text() << "\n";
			ns_hide_worm_window();
			throw;
		}
	}
	static void step_error_label(ns_death_time_annotation &  sticky_properties){
		//first clear errors where event is both excluded and a flag specified
		if (sticky_properties.is_excluded() && sticky_properties.flag.specified()){
			sticky_properties.excluded = ns_death_time_annotation::ns_not_excluded;
			sticky_properties.flag = ns_death_time_annotation_flag::none();
			return;
		}
		//first click excludes the animal
		if (!sticky_properties.is_excluded() && !sticky_properties.flag.specified()){
			sticky_properties.excluded = ns_death_time_annotation::ns_by_hand_excluded;
			return;
		}
		//second click triggers a flag.
		if (sticky_properties.is_excluded())
			sticky_properties.excluded = ns_death_time_annotation::ns_not_excluded;
		sticky_properties.flag.step_event();
		
	}
	static std::string pad_zeros(std::string & str, int len){
		int add = len-(int)str.size();
		if (add <= 0)
			return str;
		std::string ret;
		for (int i = 0; i < add; i++){
			ret+='0';
		}
		return ret+str;
	}
	void output_worm_frames(const string &base_dir,const std::string & filename, ns_sql & sql){
		const string bd(base_dir);
		const string base_directory(bd + DIR_CHAR_STR + filename);
		ns_dir::create_directory_recursive(base_directory);
		const string gray_directory(base_directory + DIR_CHAR_STR + "gray");
		const string movement_directory(base_directory + DIR_CHAR_STR + "movement");
		ns_dir::create_directory_recursive(gray_directory);
		ns_dir::create_directory_recursive(movement_directory);
	
		current_region_data->load_movement_analysis(current_region_data->metadata.region_id,sql,true);
		current_worm = &current_region_data->movement_analyzer[properties_for_all_animals.stationary_path_id.group_id].paths[properties_for_all_animals.stationary_path_id.path_id];
		
		//add in by hand annotations so they are outputted correctly.
		std::vector<ns_death_time_annotation> by_hand_annotations;
		for (unsigned long i = 0; i < current_region_data->by_hand_timing_data.size(); i++){
				current_region_data->by_hand_timing_data[i].animals.begin()->generate_event_timing_data(by_hand_annotations);
				ns_death_time_annotation_compiler c;
				for (unsigned int j = 0; j < by_hand_annotations.size(); j++)
					c.add(by_hand_annotations[j],current_region_data->metadata);
				current_region_data->movement_analyzer.add_by_hand_annotations(c);
		}
		
		
		//we're reallocating all the elements so we need to link them in.
		for (unsigned int i = 0; i < timepoints.size(); i++){
				timepoints[i].path_timepoint_element = &current_worm->element(i);
		}
		const std::string outfile(base_directory + DIR_CHAR_STR + filename + "_movement_quantification.csv");
		ofstream o(outfile.c_str());
		current_region_data->movement_analyzer.group(properties_for_all_animals.stationary_path_id.group_id).paths.begin()->write_detailed_movement_quantification_analysis_header(o);
		o << "\n";
		current_region_data->movement_analyzer.write_detailed_movement_quantification_analysis_data(current_region_data->metadata,o,false,properties_for_all_animals.stationary_path_id.group_id);
		o.close();


	//	string base_filename(base_directory + DIR_CHAR_STR + filename);
		ns_image_standard mvt_tmp;
		current_region_data->load_images_for_worm(properties_for_all_animals.stationary_path_id,current_element_id()+1,sql);
		unsigned long num_dig = ceil(log10((double)this->current_element_id()));

		for (unsigned int i = 0; i <= this->current_element_id(); i++){
			if (timepoints[i].path_timepoint_element->excluded)
				continue;
			const ns_image_standard * im(current_region_data->movement_analyzer.group(properties_for_all_animals.stationary_path_id.group_id).paths[properties_for_all_animals.stationary_path_id.path_id].element(i).image_p());
			if (im == 0)
				continue;
			//ns_image_properties prop(im.properties());
			ns_save_image(gray_directory + DIR_CHAR_STR + filename + "_grayscale_" + ns_to_string(i+1) + ".tif",*im);
			current_region_data->movement_analyzer.group(properties_for_all_animals.stationary_path_id.group_id).paths[properties_for_all_animals.stationary_path_id.path_id].element(i).generate_movement_visualization(mvt_tmp);
			//ns_image_properties prop(im.properties()
			ns_save_image(movement_directory + DIR_CHAR_STR + filename +  "_" + ns_to_string(i+1) + ".tif",mvt_tmp);
		}
		
		
	}
	
	std::vector<ns_death_time_annotation> click_event_cache;
	void register_click(const ns_vector_2i & image_position,const ns_click_request & action){
		ns_acquire_lock_for_scope lock(image_buffer_access_lock,__FILE__,__LINE__);
		const unsigned long hand_bar_group_bottom(bottom_margin_position().y);

		const unsigned long hand_bar_height((current_by_hand_timing_data().animals.size()+1)*ns_death_time_solo_posture_annotater_timepoint::ns_movement_bar_height);
		
		
		bool change_made = false;
		bool click_handled_by_hand_bar_choice(false);
		if (action == ns_cycle_state &&
			image_position.y >= hand_bar_group_bottom && 
			image_position.y < hand_bar_group_bottom+hand_bar_height){
			const unsigned long all_bar_id( (image_position.y - hand_bar_group_bottom)/ns_death_time_solo_posture_annotater_timepoint::ns_movement_bar_height);
			if (all_bar_id > 0){
				const unsigned long hand_bar_id = all_bar_id-1;
				if (hand_bar_id > current_by_hand_timing_data().animals.size())
					throw ns_ex("Invalid hand bar");
				if (hand_bar_id != current_animal_id){
					current_animal_id = hand_bar_id;
					change_made = true;
					click_handled_by_hand_bar_choice = true;
				}
			}
			else{
				ns_vector_2i bottom_margin_bottom(bottom_margin_position());
				const ns_time_path_limits observation_limit(current_worm->observation_limits());

				unsigned long requested_time = current_machine_timing_data->animals[0].get_time_from_movement_diagram_position(image_position.x,bottom_margin_bottom,
												ns_vector_2i(current_worm->element(current_element_id()).image().properties().width,0),
												observation_limit);
				clear_cached_images();
				set_current_timepoint(requested_time,false);
				{
					ns_image_standard temp_buffer;
					timepoints[current_timepoint_id].load_image(1024,current_image,sql(),temp_buffer,1);
				}
				change_made = true;

				click_handled_by_hand_bar_choice = true;
			}
		}
		if (!click_handled_by_hand_bar_choice){
			switch(action){
				case ns_cycle_state:  current_by_hand_timing_data().animals[current_animal_id].step_event(
										  ns_death_timing_data_step_event_specification(
										  current_time_interval(),current_worm->element(current_element_id()),
										  properties_for_all_animals.region_info_id,properties_for_all_animals.stationary_path_id,current_animal_id),current_worm->observation_limits());
					change_made = true;
					break;
				case ns_cycle_flags: 
					step_error_label(properties_for_all_animals);
					change_made = true;
					break;
				case ns_annotate_extra_worm:
					{
					unsigned long & current_annotated_worm_count(properties_for_all_animals.number_of_worms_at_location_marked_by_hand);
				
					if (current_by_hand_timing_data().animals.size() >= ns_death_time_annotation::maximum_number_of_worms_at_position){
						current_by_hand_timing_data().animals.resize(1);
						current_annotated_worm_count = 1;
						current_animal_id = 0;
					}
					else{
						const unsigned long new_animal_id(current_by_hand_timing_data().animals.size());
						current_by_hand_timing_data().animals.resize(new_animal_id+1);
						current_by_hand_timing_data().animals.rbegin()->set_fast_movement_cessation_time(ns_death_timing_data_step_event_specification( 
							ns_death_timing_data_step_event_specification(
										  current_time_interval(),
										  current_worm->element(current_element_id()),
											properties_for_all_animals.region_info_id,
											properties_for_all_animals.stationary_path_id,new_animal_id)));
						current_by_hand_timing_data().animals.rbegin()->animal_specific_sticky_properties.animal_id_at_position = new_animal_id;
						//add a "object has stopped fast moving" event at first timepoint of new path
						current_by_hand_timing_data().animals.rbegin()->step_event(
										  ns_death_timing_data_step_event_specification(
										  current_time_interval(),current_worm->element(current_element_id()),
										  properties_for_all_animals.region_info_id,properties_for_all_animals.stationary_path_id,new_animal_id),current_worm->observation_limits());
						this->current_animal_id = new_animal_id;
						unsigned long new_sticky_label =current_by_hand_timing_data().animals.size();
						if (current_annotated_worm_count > new_sticky_label)
							new_sticky_label = current_annotated_worm_count;
						properties_for_all_animals.number_of_worms_at_location_marked_by_hand = new_sticky_label;
						
					}
					//current_by_hand_timing_data->animals[current_animal_id].annotate_extra_worm(); 
					change_made = true;
					break;
				}
				case ns_increase_contrast:
					dynamic_range_rescale_factor+=.1; 
					change_made = true; 
					break;
				case ns_decrease_contrast:{
					dynamic_range_rescale_factor-=.1; 
					if (dynamic_range_rescale_factor < .1)
						dynamic_range_rescale_factor = .1;
					change_made = true; 
					break;
					}

				case ns_output_images:{
					bool in_char(false);
					const string pn(current_region_data->metadata.plate_name());
					std::string plate_name;
					for (unsigned int i = 0; i < pn.size(); i++){
						if (pn[i] == ':'){
							if (in_char)
								continue;
							else{
								plate_name += "_";
								in_char = true;
								continue;
							}
						}
						plate_name+= pn[i];
						in_char = false;
					}
					const string filename(this->current_region_data->metadata.experiment_name + "=" + plate_name  + "=" + ns_to_string(properties_for_all_animals.stationary_path_id.group_id));
					if (filename == "")
						break;
					
					ns_file_chooser d;
					d.dialog_type = Fl_Native_File_Chooser::BROWSE_DIRECTORY;
					d.default_filename = "";
					d.title = "Choose Movement Quantification Output Directory";
					d.act();
				//	ns_run_in_main_thread<ns_file_chooser> run_mt(&d);
					if (d.chosen)
						output_worm_frames(d.result,filename,sql());
					break;
									  }
				default: throw ns_ex("ns_death_time_posture_annotater::Unknown click type");
			}
		}
		if (change_made){
			update_events_to_storyboard();
			lock.release();
			draw_metadata(&timepoints[current_timepoint_id],*current_image.im);
			request_refresh();
		}
		else 
		lock.release();
		

	}

	void display_current_frame();
	private:
	void update_events_to_storyboard(){
		properties_for_all_animals.annotation_time = ns_current_time();
		current_by_hand_timing_data().animals[current_animal_id].specified = true;
		click_event_cache.resize(0);
		for (unsigned int i = 0; i < current_by_hand_timing_data().animals.size(); i++){
			current_by_hand_timing_data().animals[i].generate_event_timing_data(click_event_cache);
			current_by_hand_timing_data().animals[i].specified = true;
		}
		ns_specifiy_worm_details(properties_for_all_animals.region_info_id,properties_for_all_animals.stationary_path_id,properties_for_all_animals,click_event_cache);
	}
		
	ns_alignment_type alignment_type;
};

#endif
