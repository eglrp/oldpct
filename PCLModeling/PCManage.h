#pragma once

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/PointIndices.h>
#include <boost/make_shared.hpp>
#include <vector>

typedef pcl::PointXYZRGB PointT;
typedef pcl::PointCloud<PointT> PointCloudT;

class PCManage
{
public:
	PointCloudT::Ptr cloud_;			//��������
	std::vector <pcl::PointIndices> jlClusters_;	//������

	static PCManage &ins();
private:
	PCManage();
};

