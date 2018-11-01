#include "ProjectionXyPlane.h"
#include "QRendView.h"
#include <pcl/ModelCoefficients.h>
#include <pcl/filters/project_inliers.h>

ProjectionXyPlane::ProjectionXyPlane()
{
}


ProjectionXyPlane::~ProjectionXyPlane()
{
}

void ProjectionXyPlane::Fun()
{
	QRendView* ins = QRendView::MainRendView();
	PointCloudT::Ptr cloud = PCManage::ins().cloud_;
	pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients());
	coefficients->values.resize(4);
	coefficients->values[0] = coefficients->values[1] = 0;
	coefficients->values[2] = 1.0;
	coefficients->values[3] = 0;
	pcl::ProjectInliers<pcl::PointXYZRGB> proj;      //����ͶӰ�˲�����
	proj.setModelType(pcl::SACMODEL_PLANE);      //���ö����Ӧ��ͶӰģ��
	proj.setInputCloud(cloud);                    //�����������
	proj.setModelCoefficients(coefficients);     //����ģ�Ͷ�Ӧ��ϵ��
	proj.filter(*cloud);                //ִ��ͶӰ�˲��洢���cloud_projected
	ins->UpdateView();
}

void ProjectionXyPlane::Fun(PointCloudT::Ptr cloud)
{
    pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients());
    coefficients->values.resize(4);
    coefficients->values[0] = coefficients->values[1] = 0;
    coefficients->values[2] = 1.0;
    coefficients->values[3] = 0;
    pcl::ProjectInliers<pcl::PointXYZRGB> proj;      //����ͶӰ�˲�����
    proj.setModelType(pcl::SACMODEL_PLANE);      //���ö����Ӧ��ͶӰģ��
    proj.setInputCloud(cloud);                    //�����������
    proj.setModelCoefficients(coefficients);     //����ģ�Ͷ�Ӧ��ϵ��
    proj.filter(*cloud);                //ִ��ͶӰ�˲��洢���cloud_projected
}

