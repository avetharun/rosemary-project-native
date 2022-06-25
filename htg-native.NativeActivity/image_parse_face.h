#pragma once

#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
class image_parse_face
{
	// We need a face detector.  We will use this to get bounding boxes for
	// each face in an image.
	static inline dlib::frontal_face_detector face_detector = dlib::get_frontal_face_detector();
	// And we also need a shape_predictor.  This is the tool that will predict face
	// landmark positions given an image and face bounding box.  Here we are just
	// loading the model from the shape_predictor_68_face_landmarks.dat file you gave
	// as a command line argument.

	static inline dlib::shape_predictor shape_predictor;

	image_parse_face() {}

};

