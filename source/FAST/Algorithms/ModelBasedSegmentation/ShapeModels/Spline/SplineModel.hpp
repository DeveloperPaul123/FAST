#ifndef SPLINE_MODEL_HPP
#define SPLINE_MODEL_HPP

#include "FAST/Algorithms/ModelBasedSegmentation/ShapeModel.hpp"

namespace fast {

class SplineModel : public ShapeModel {
	FAST_OBJECT(SplineModel)
	public:
		Shape::pointer getShape(VectorXf state);
		MatrixXf getStateTransitionMatrix1();
		MatrixXf getStateTransitionMatrix2();
		MatrixXf getStateTransitionMatrix3();
		MatrixXf getProcessErrorMatrix();
		VectorXf getInitialState(SharedPointer<Image> image);
		std::vector<MatrixXf> getMeasurementVectors(VectorXf state, Shape::pointer shape);
		void initializeShapeToImageCenter();
	private:
		SplineModel();
		std::vector<Vector2f> getLocallyDeformedVertices(VectorXf state);

		MatrixXf mA1;
		MatrixXf mA2;
		MatrixXf mA3;
		MatrixXf mProcessErrorMatrix;
		int mStateSize;
		int mResolution;

		std::vector<Vector2f> mControlPoints;
		float mTension;
		Vector2f mCentroid;

		bool mInitializeShapeToImageCenter;

};

}

#endif
