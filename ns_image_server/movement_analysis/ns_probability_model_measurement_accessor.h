#pragma once
//this header file defines constructs shared between the hidden markov model analysis implementation and poster analysis cross validation implementations, but hidden from their interfaces
#include "ns_analyzed_image_time_path_element_measurements.h"
#include "ns_movement_state.h"
#include "ns_gmm.h"

struct ns_measurement_accessor {
	typedef ns_hmm_emission data_t;
	virtual const double operator()(const ns_analyzed_image_time_path_element_measurements& e, bool& valid_point) const = 0;
	virtual ns_measurement_accessor* clone() = 0;
	const double operator()(const data_t& e, bool& valid_point) const { return (*this)(e.measurement, valid_point); }
	const double get_from_emission(const data_t& e, bool& valid_point) const { return (*this)(e.measurement, valid_point); }
};

struct ns_duration_accessor {
	typedef ns_hmm_duration data_t;
	const double operator()(const double& e, bool& valid_point) const {
		valid_point = true; 
		return log(e+1);
	}
	ns_duration_accessor* clone() {
		return new ns_duration_accessor;
	}
	const double operator()(const data_t& e, bool& valid_point) const {
		return (*this)(e.measurement, valid_point);
	}
};

class ns_probability_model_holder {
public:
	std::map<ns_hmm_movement_state, ns_hmm_probability_model<ns_measurement_accessor>*> state_emission_models;
	std::map<ns_hmm_state_transition, ns_hmm_probability_model<ns_duration_accessor>*> state_transition_models;
	~ns_probability_model_holder() {
		for (auto p = state_emission_models.begin(); p != state_emission_models.end(); ++p)
			delete p->second;
		for (auto p = state_transition_models.begin(); p != state_transition_models.end(); ++p)
			delete p->second;
	}
};

struct ns_intensity_accessor_1x : public ns_measurement_accessor {
	const double operator()(const ns_analyzed_image_time_path_element_measurements& e, bool& valid_point) const {
		valid_point = true;
		return e.change_in_total_stabilized_intensity_1x / 100.0;
	}
	const double get_from_emission(const ns_hmm_emission& e, bool& valid_point) const { return (*this)(e.measurement, valid_point); }
	ns_measurement_accessor* clone() { return new ns_intensity_accessor_1x; }
};
struct ns_intensity_accessor_2x : public ns_measurement_accessor {
	const double operator()(const ns_analyzed_image_time_path_element_measurements& e, bool& valid_point) const {
		valid_point = true;
		return e.change_in_total_stabilized_intensity_2x / 100.0;
	}
	const double get_from_emission(const ns_hmm_emission& e, bool& valid_point) const { return (*this)(e.measurement, valid_point); }
	ns_measurement_accessor* clone() { return new ns_intensity_accessor_2x; }
};
struct ns_intensity_accessor_4x : public ns_measurement_accessor {
	const double operator()(const ns_analyzed_image_time_path_element_measurements& e, bool& valid_point) const {
		valid_point = true;
		return e.change_in_total_stabilized_intensity_4x / 100.0;
	}
	const double get_from_emission(const ns_hmm_emission& e, bool& valid_point) const { return (*this)(e.measurement, valid_point); }
	ns_measurement_accessor* clone() { return new ns_intensity_accessor_4x; }
};

struct ns_movement_accessor : public ns_measurement_accessor {
	const double operator()(const ns_analyzed_image_time_path_element_measurements& e, bool& valid_point) const {
		//this defines the movement score used by the HMM model!
		valid_point = true;
		const double d = e.death_time_posture_analysis_measure_v2_uncropped() + .1;
		if (d <= 0) return -DBL_MAX;
		return log(d);
	}
	const double get_from_emission(const ns_hmm_emission& e, bool& valid_point) const { return (*this)(e.measurement, valid_point); }
	ns_measurement_accessor* clone() { return new ns_movement_accessor; }
};

template<int crop_level>
struct ns_cropped_movement_accessor : public ns_measurement_accessor {
	const double operator()(const ns_analyzed_image_time_path_element_measurements& e, bool& valid_point) const {
		//this defines the movement score used by the HMM model!
		valid_point = true;
		const double d = e.spatial_averaged_movement_sum_cropped[crop_level] + .1;
		if (d <= 0) return -DBL_MAX;
		return log(d);
	}
	const double get_from_emission(const ns_hmm_emission& e, bool& valid_point) const { return (*this)(e.measurement, valid_point); }
	ns_measurement_accessor* clone() { return new ns_cropped_movement_accessor<crop_level>; }
};


struct ns_cropped_movement_accessor_4x : public ns_measurement_accessor {
	const double operator()(const ns_analyzed_image_time_path_element_measurements& e, bool& valid_point) const {
		//this defines the movement score used by the HMM model!
		valid_point = true;
		const double d = e.spatial_averaged_movement_sum_cropped_4x + .1;
		if (d <= 0) return -DBL_MAX;
		return log(d);
	}
	const double get_from_emission(const ns_hmm_emission& e, bool& valid_point) const { return (*this)(e.measurement, valid_point); }
	ns_measurement_accessor* clone() { return new ns_cropped_movement_accessor_4x; }
};
struct ns_scaled_movement_accessor : public ns_measurement_accessor {
	const double operator()(const ns_analyzed_image_time_path_element_measurements& e, bool& valid_point) const {
		//this defines the movement score used by the HMM model!
		valid_point = true;
		const double d = e.spatial_averaged_scaled_movement_sum_cropped + .1;
		if (d <= 0) return -DBL_MAX;
		return log(d);
	}
	const double get_from_emission(const ns_hmm_emission& e, bool& valid_point) const { return (*this)(e.measurement, valid_point); }
	ns_measurement_accessor* clone() { return new ns_scaled_movement_accessor; }
};
struct ns_scaled_movement_accessor_4x : public ns_measurement_accessor {
	const double operator()(const ns_analyzed_image_time_path_element_measurements& e, bool& valid_point) const {
		//this defines the movement score used by the HMM model!
		valid_point = true;
		const double d = e.spatial_averaged_scaled_movement_sum_cropped_4x + .1;
		if (d <= 0) return -DBL_MAX;
		return log(d);
	}
	const double get_from_emission(const ns_hmm_emission& e, bool& valid_point) const { return (*this)(e.measurement, valid_point); }
	ns_measurement_accessor* clone() { return new ns_scaled_movement_accessor_4x; }
};
/*
struct ns_movement_accessor_min_4x : public ns_measurement_accessor {
	const double operator()(const ns_analyzed_image_time_path_element_measurements& e, bool& valid_point) const {
		//this defines the movement score used by the HMM model!
		valid_point = true;
		const double d = e.spatial_averaged_movement_score_uncropped_min_4x + .1;
		if (d <= 0) return -DBL_MAX;
		return log(d);
	}
	const double get_from_emission(const ns_hmm_emission& e, bool& valid_point) const { return (*this)(e.measurement, valid_point); }
	ns_measurement_accessor* clone() { return new ns_movement_accessor; }
};*/

struct ns_outside_intensity_accessor_1x : public ns_measurement_accessor {
	const double operator()(const ns_analyzed_image_time_path_element_measurements& e, bool& valid_point) const {
		valid_point = true;
		return e.change_in_total_outside_stabilized_intensity_1x / 100.0;
	}
	const double get_from_emission(const ns_hmm_emission& e, bool& valid_point) const { return (*this)(e.measurement, valid_point); }
	ns_measurement_accessor* clone() { return new ns_outside_intensity_accessor_1x; }
};
struct ns_outside_intensity_accessor_2x : public ns_measurement_accessor {
	const double operator()(const ns_analyzed_image_time_path_element_measurements& e, bool& valid_point) const {
		valid_point = true;
		return e.change_in_total_outside_stabilized_intensity_2x / 100.0;
	}
	const double get_from_emission(const ns_hmm_emission& e, bool& valid_point) const { return (*this)(e.measurement, valid_point); }
	ns_measurement_accessor* clone() { return new ns_outside_intensity_accessor_2x; }
};
struct ns_outside_intensity_accessor_4x : public ns_measurement_accessor {
	const double operator()(const ns_analyzed_image_time_path_element_measurements& e, bool& valid_point) const {
		valid_point = true;
		return e.change_in_total_outside_stabilized_intensity_4x / 100.0;
	}
	const double get_from_emission(const ns_hmm_emission& e, bool& valid_point) const { return (*this)(e.measurement, valid_point); }
	ns_measurement_accessor* clone() { return new ns_outside_intensity_accessor_4x; }
};

struct ns_dummy_accessor : public ns_measurement_accessor {
	const double operator()(const ns_analyzed_image_time_path_element_measurements& e, bool& valid_point) const {
		throw ns_ex("Accessing dummy accessor!");
	}
	const double get_from_emission(const ns_hmm_emission& e, bool& valid_point) const { return (*this)(e.measurement, valid_point); }
	ns_measurement_accessor* clone() { return new ns_dummy_accessor; }
};


struct ns_stabilized_region_vs_outside_intensity_comparitor : public ns_measurement_accessor {
	const double operator()(const ns_analyzed_image_time_path_element_measurements& e, bool& valid_point) const {
		valid_point = true;
		return (e.change_in_total_outside_stabilized_intensity_2x - e.change_in_total_stabilized_intensity_2x) / 100.0;
	}
	const double get_from_emission(const ns_hmm_emission& e, bool& valid_point) const { return (*this)(e.measurement, valid_point); }
	ns_measurement_accessor* clone() { return new ns_stabilized_region_vs_outside_intensity_comparitor; }
};


class ns_hmm_emission_probability_model_organizer : public ns_hmm_probability_model_organizer<ns_measurement_accessor> {
public:

	ns_covarying_gaussian_dimension<ns_measurement_accessor> create_dimension(const std::string& d) const {
		if (d == "m") return ns_covarying_gaussian_dimension<ns_measurement_accessor>(new ns_movement_accessor, d);
		if (d == "c0") return ns_covarying_gaussian_dimension<ns_measurement_accessor>(new ns_cropped_movement_accessor<0>, d);
		if (d == "c1") return ns_covarying_gaussian_dimension<ns_measurement_accessor>(new ns_cropped_movement_accessor<1>, d);
		if (d == "c2") return ns_covarying_gaussian_dimension<ns_measurement_accessor>(new ns_cropped_movement_accessor<2>, d);
		if (d == "c0_4x") return ns_covarying_gaussian_dimension<ns_measurement_accessor>(new ns_cropped_movement_accessor_4x, d);
		if (d == "s") return ns_covarying_gaussian_dimension<ns_measurement_accessor>(new ns_scaled_movement_accessor, d);
		if (d == "s_4x") return ns_covarying_gaussian_dimension<ns_measurement_accessor>(new ns_scaled_movement_accessor_4x, d);
		if (d == "i1") return ns_covarying_gaussian_dimension<ns_measurement_accessor>(new ns_intensity_accessor_1x, d);
		if (d == "i2") return ns_covarying_gaussian_dimension<ns_measurement_accessor>(new ns_intensity_accessor_2x, d);
		if (d == "i4") return ns_covarying_gaussian_dimension<ns_measurement_accessor>(new ns_intensity_accessor_4x, d);
		if (d == "o1") return ns_covarying_gaussian_dimension<ns_measurement_accessor>(new ns_outside_intensity_accessor_1x, d);
		if (d == "o2") return ns_covarying_gaussian_dimension<ns_measurement_accessor>(new ns_outside_intensity_accessor_2x, d);
		if (d == "o4") return ns_covarying_gaussian_dimension<ns_measurement_accessor>(new ns_outside_intensity_accessor_4x, d);
		if (d == "c") return ns_covarying_gaussian_dimension<ns_measurement_accessor>(new ns_stabilized_region_vs_outside_intensity_comparitor, d);
		//state transition models are first read into state emission models, and then converted.
		if (d == "dur") return ns_covarying_gaussian_dimension<ns_measurement_accessor>(new ns_dummy_accessor, d);
		throw ns_ex("ns_emission_probability_model_organizer()::Unknown dimension: ") << d;
	}
	static std::string dimension_description(const std::string& d)  {
		if (d == "m") return "Un-Cropped Spatially Averaged Movement Sum";
		if (d == "c0") return "Spatially Averaged Movement Sum Cropped at level 0";
		if (d == "c1") return "Spatially Averaged Movement Sum Cropped at level 1";
		if (d == "c2") return "Spatially Averaged Movement Sum Cropped at level 2";
		if (d == "c0_4x") return "4x temporally smoothed, Spatially Averaged Movement Sum Cropped at level 0";
		if (d == "s") return "Scaled, Un-cropped Spatially Averaged Movement Sum";
		if (d == "s_4x") return "Temporally smoothed, Scaled, Un-cropped Spatially Averaged Movement Sum";
		if (d == "i1") return "Change in total intensity around the worm";
		if (d == "i2") return "2x Temporally smoothed, Change in total intensity around the worm";
		if (d == "i4") return "4x Temporally smoothed, Change in total intensity around the worm";
		if (d == "o1") return "Change in total intensity far from the worm";
		if (d == "o2") return "2x Temporally smoothed, Change in total intensity far from the worm";
		if (d == "o4") return "4x Temporally smoothed, Change in total intensity far from the worm";
		if (d == "c") return  "Ratio between betweeen total intensity close and total intensity far from the worm";
		//state transition models are first read into state emission models, and then converted.
		if (d == "dur") return "Dummy Dimension used to store state durations";
		throw ns_ex("ns_emission_probability_model_organizer()::Unknown dimension: ") << d;
	}

	ns_emission_probabiliy_gaussian_diagonal_covariance_model<ns_measurement_accessor>* default_model() const {
		auto p = new ns_emission_probabiliy_gaussian_diagonal_covariance_model<ns_measurement_accessor>;
		p->dimensions.insert(p->dimensions.end(), create_dimension("m"));
		p->dimensions.insert(p->dimensions.end(), create_dimension("m4"));
		p->dimensions.insert(p->dimensions.end(), create_dimension("i1"));
		p->dimensions.insert(p->dimensions.end(), create_dimension("i4"));
		p->setup_gmm(p->dimensions.size(), 4);
		return p;
	}

};


//generates probability models with the specified dimensions.  passed to the hmm framework during model building
class ns_probability_model_generator {
public:
	ns_probability_model_generator(const std::vector<std::string>& dimensions_) :dimensions(&dimensions_) {}
	ns_emission_probabiliy_gaussian_diagonal_covariance_model<ns_measurement_accessor>* operator()() const {
		ns_hmm_emission_probability_model_organizer org;
		auto p = new ns_emission_probabiliy_gaussian_diagonal_covariance_model<ns_measurement_accessor>;
		for (unsigned int i = 0; i < dimensions->size(); i++)
			p->dimensions.insert(p->dimensions.end(), org.create_dimension((*dimensions)[i]));
		p->setup_gmm(p->dimensions.size(), dimensions->size());
	return p;
	}
private:
	const std::vector<std::string>* dimensions;

};
