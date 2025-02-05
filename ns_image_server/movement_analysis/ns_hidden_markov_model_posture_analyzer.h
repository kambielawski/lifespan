#ifndef NS_HIDDEN_MARKOV_MODEL_POSTURE_ANALYZER_H
#define NS_HIDDEN_MARKOV_MODEL_POSTURE_ANALYZER_H

#include "ns_ex.h"
#include "ns_death_time_annotation.h"
#include "ns_time_path_posture_movement_solution.h"
#include "ns_posture_analysis_models.h"
#include <limits.h>
#include "ns_region_metadata.h"

struct ns_hmm_movement_analysis_optimizatiom_stats_event_annotation {
	ns_death_time_annotation_time_interval by_hand, machine;
	bool by_hand_identified, machine_identified;
};

struct ns_hmm_movement_optimization_stats_record_path_element {
	ns_hmm_movement_state state;
	double total_log_probability;
	double state_transition_log_probability;
	std::vector<double> log_sub_probabilities;
	std::vector<double> sub_measurements;
};
struct ns_hmm_movement_optimization_stats_record_path {
	double log_likelihood;
	//state,log-likelihood of path up until that state
	std::vector<ns_hmm_movement_optimization_stats_record_path_element> path;
};
struct ns_hmm_movement_analysis_optimizatiom_stats_record {
	enum { number_of_states = 6 };
	static const ns_movement_event states[number_of_states];
	typedef std::map<ns_movement_event, ns_hmm_movement_analysis_optimizatiom_stats_event_annotation> ns_record_list;
	ns_record_list measurements;
	ns_stationary_path_id path_id;
	ns_death_time_annotation properties;
	const std::string* database_name;
	double solution_loglikelihood;
	
	ns_hmm_movement_optimization_stats_record_path by_hand_state_info, machine_state_info;
	std::vector<double> state_info_times;
	std::vector<std::string> state_info_variable_names;

};

struct ns_hmm_movement_analysis_optimizatiom_stats {
	std::vector<ns_hmm_movement_analysis_optimizatiom_stats_record> animals;

	static void write_error_header(std::ostream & o, const std::vector<std::string> & extra_columns);
	void write_error_data(const std::string & analysis_approach, const std::vector<std::string>& measurement_names_to_write,std::ostream & o, const std::string& genotype_set, const std::string & cross_validation_info, const unsigned long & cross_validation_replicate_id, const std::map<std::string, std::map<ns_64_bit, ns_region_metadata> > & metadata_cache) const;

	void write_hmm_path_header(std::ostream & o) const;
	void write_hmm_path_data(const std::string& analysis_approach,std::ostream & o, const std::map<std::string, std::map<ns_64_bit, ns_region_metadata> > & metadata_cache) const;

};
typedef ns_analyzed_image_time_path_death_time_estimator_reusable_memory ns_hmm_solver_reusable_memory;

class ns_hmm_solver {
public:


	typedef std::pair<ns_hmm_movement_state, unsigned long> ns_hmm_state_transition_time_path_index;

	ns_time_path_posture_movement_solution movement_state_solution;

	void solve(const ns_analyzed_image_time_path& path, const ns_emperical_posture_quantification_value_estimator& estimator, ns_hmm_solver_reusable_memory& reusable_memory);
	static void probability_of_path_solution(const ns_analyzed_image_time_path & path, const ns_emperical_posture_quantification_value_estimator & estimator, const ns_time_path_posture_movement_solution & solution, ns_hmm_movement_optimization_stats_record_path  & state_info, bool generate_path_info);


	//run the viterbi algorithm using the specified indicies of the path
	static double run_viterbi(const ns_analyzed_image_time_path & path, const ns_emperical_posture_quantification_value_estimator & estimator, const std::vector<unsigned long> & path_indices,
		std::vector<ns_hmm_state_transition_time_path_index > &movement_transitions, ns_hmm_solver_reusable_memory& mem);

	void build_movement_state_solution_from_movement_transitions(const unsigned long first_stationary_path_index,const std::vector<unsigned long> path_indices, const std::vector<ns_hmm_state_transition_time_path_index > & movement_transitions);
	
	//m[i][j] is the bais for or against an individual in state i transitioning to state j.
	//the actual probabilities are calculated from empiric transition probabilities multiplied by these weights
	static void build_state_transition_weight_matrix(const ns_emperical_posture_quantification_value_estimator & estimator, std::vector<std::vector<double> > & m);

	static void output_state_transition_matrix(const std::string & title, std::vector<std::vector<double> >& m, std::ostream& o);

	static ns_hmm_movement_state most_probable_state(const std::vector<double> & d);


};

class ns_time_path_movement_markov_solver : public ns_analyzed_image_time_path_death_time_estimator{
public:
	ns_time_path_movement_markov_solver(const ns_emperical_posture_quantification_value_estimator & e):estimator(e){}
	ns_time_path_posture_movement_solution operator()(const ns_analyzed_image_time_path * path, const double vigorous_movement_thresh, std::ostream * debug_output_ = 0)const {
		ns_hmm_solver_reusable_memory reusable_memory;
		return estimate_posture_movement_states(2, vigorous_movement_thresh,path, reusable_memory,0,debug_output_);
	}
	ns_time_path_posture_movement_solution operator()(const ns_analyzed_image_time_path * path, const double vigorous_movement_thresh, const bool fill_in_loglikelihood_timeseries, ns_hmm_solver_reusable_memory & reusable_memory, std::ostream * debug_output_=0)const{
		return estimate_posture_movement_states(2, vigorous_movement_thresh, path, reusable_memory,0,debug_output_);
	}
	ns_time_path_posture_movement_solution operator() (ns_analyzed_image_time_path * path, const double vigorous_movement_thresh, const bool fill_in_loglikelihood_timeseries,std::ostream * debug_output=0)const{
		ns_hmm_solver_reusable_memory reusable_memory;
		return estimate_posture_movement_states(2, vigorous_movement_thresh, path, reusable_memory,path,debug_output);
	}

	ns_time_path_posture_movement_solution estimate_posture_movement_states(int software_value,const double vigorous_movement_thresh,const ns_analyzed_image_time_path * source_path, ns_hmm_solver_reusable_memory & reusable_memory, ns_analyzed_image_time_path * output_path = 0,std::ostream * debug_output=0) const;
	const ns_emperical_posture_quantification_value_estimator & estimator;
	static std::string current_software_version() { return "2.5"; }
	std::string current_software_version_number() const { return current_software_version(); }
	std::string model_software_version_number() const;

	const std::string& model_description() const { return estimator.model_description_text;}
	unsigned long latest_possible_death_time(const ns_analyzed_image_time_path * path, const unsigned long last_observation_time) const{return  last_observation_time; }
};

#endif
