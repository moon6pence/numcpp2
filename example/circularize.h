#include "../config.h"
#include <numcpp/numcpp.h>
#include <numcpp/opencv.h>

#ifdef USE_CUDA
#include <numcpp/cuda.h>
#endif

namespace np {

struct Circularize
{
	array_t<float> x_map, y_map;

	void buildCircularizeMap(const int HEIGHT, const int WIDTH, const int DIAMETER)
	{
		const float PI = 3.1415927f;
		
		array_t<float> X, Y;
		array_t<float> theta, rho;
		
		meshgrid(X, Y, colon(0.f, DIAMETER - 1.f), colon(0.f, DIAMETER - 1.f));

		const int X0 = DIAMETER / 2, Y0 = DIAMETER / 2;
		X = X - (float)X0;
		Y = Y - (float)Y0;

		// theta
		theta = atan2(Y, X);

		// X map: interpolate (-pi, pi) -> (0, width - 1)
		x_map = (theta + PI) * ((WIDTH-1) / (2*PI));

		// rho
		rho = sqrt(X * X + Y * Y);

		// Y map: interpolate (0, RADIUS) -> (1, height)
		y_map = rho * ((float)(HEIGHT-1) / (DIAMETER/2));
	}

	void operator() (array_t<uint8_t> &dst, const array_t<uint8_t> &src, int DIAMETER)
	{
		dst.setSize(DIAMETER, DIAMETER);

		if (x_map.empty() || y_map.empty())
			buildCircularizeMap(src.size(0), src.size(1), DIAMETER);

		cv::remap(to_cv_mat(src), to_cv_mat(dst), to_cv_mat(x_map), to_cv_mat(y_map), CV_INTER_LINEAR);
	}

};

#ifdef USE_CUDA

struct Circularize_d : private Circularize
{
	device_array_t<float> x_map_d, y_map_d;

	void operator() (device_array_t<uint8_t> &dst, const device_array_t<uint8_t> &src, int DIAMETER)
	{
		dst.setSize(DIAMETER, DIAMETER);

		if (x_map.empty() || y_map.empty())
			buildCircularizeMap(src.size(0), src.size(1), DIAMETER);

		if (x_map_d.empty() || y_map_d.empty())
		{
			x_map_d = device_array_t<float>(x_map);
			y_map_d = device_array_t<float>(y_map);
		}

		cv::gpu::GpuMat cv_dst = to_cv_gpu_mat(dst);
		cv::gpu::remap(to_cv_gpu_mat(src), cv_dst, to_cv_gpu_mat(x_map_d), to_cv_gpu_mat(y_map_d), CV_INTER_LINEAR);
	}
};

#endif // USE_CUDA

} // namespace np