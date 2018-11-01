#include "QQuyuzengzhangColor.h"
#include <pcl/search/search.h>
#include <pcl/segmentation/region_growing_rgb.h>
#include <pcl/features/normal_3d_omp.h>
#include "QRendView.h"
#include "PCManage.h"
#include "dvprint.h"

QQuyuzengzhangColor::QQuyuzengzhangColor(QWidget *parent)
	: QSubDialogBase(parent)
{
	ui.setupUi(this);
	ui.lineEdit->setValidator(new QDoubleValidator(this));
	ui.lineEdit_2->setValidator(new QDoubleValidator(this));
	ui.lineEdit_3->setValidator(new QDoubleValidator(this));
	ui.lineEdit_4->setValidator(new QIntValidator(this));

	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  
	ui.lineEdit->setText(QString::fromLocal8Bit(  pt.get<std::string>("��������color.���뷧ֵ").c_str()));
	ui.lineEdit_2->setText(QString::fromLocal8Bit(pt.get<std::string>("��������color.��ɫ�ֵ").c_str()));
	ui.lineEdit_3->setText(QString::fromLocal8Bit(pt.get<std::string>("��������color.��ɫ�ֵ").c_str()));
	ui.lineEdit_4->setText(QString::fromLocal8Bit(pt.get<std::string>("��������color.��С��������").c_str()));
}

QQuyuzengzhangColor::~QQuyuzengzhangColor()
{
}

void QQuyuzengzhangColor::OnApply()
{
	QRendView* ins = QRendView::MainRendView();
	PointCloudT::Ptr cloud = PCManage::ins().cloud_;

	std::string       distanceThreshold = ui.lineEdit->text().toLocal8Bit().data();
	std::string       pointColorThreshold = ui.lineEdit_2->text().toLocal8Bit().data();
	std::string       regionColorThreshold = ui.lineEdit_3->text().toLocal8Bit().data();
	std::string       minClusterSize = ui.lineEdit_4->text().toLocal8Bit().data();

	pcl::search::Search<PointT>::Ptr tree (new pcl::search::KdTree<PointT>);
	//	pcl::IndicesPtr indices(new std::vector <int>);
	pcl::RegionGrowingRGB<PointT> reg;//����������ɫ�����������ָ���Ķ���
	reg.setInputCloud(cloud);//���÷ָ�ԭʼ����
	//	reg.setIndices(indices);//���÷ָ������������
	reg.setSearchMethod(tree);//�����������������������
	reg.setDistanceThreshold(/*10*/atof(distanceThreshold.c_str()));//���þ�����ֵ��С�ڸ�ֵ����Ϊ�����
	reg.setPointColorThreshold(/*8*/atof(pointColorThreshold.c_str()));//���õ�֮���ɫ����ֵ��С�ڸ�ֵ����Ϊһ������
	reg.setRegionColorThreshold(/*15*/atof(regionColorThreshold.c_str()));//���þ���֮���ɫ����ֵ��С�ڸ�ֵ��Ӧ�úϲ��㷨���ϲ�Ϊͬһ������
	reg.setMinClusterSize(/*200*/atof(minClusterSize.c_str()));//���þ����е���������ޣ�������������ڸ�ֵ��Ӧ�úϲ��㷨���ϲ�������ٵ�һ������

// 	std::vector <pcl::PointIndices> clusters;
// 	reg.extract(clusters);//Ӧ�÷ָ��㷨����ȡ����
	


	reg.extract(PCManage::ins().jlClusters_);
    PointCloudT::Ptr colored_cloud = reg.getColoredCloud();//�Ծ��������һ��ɫ����ͬ����ɫΪ��ͬ�ľ���
	pcl::copyPointCloud(*colored_cloud, *cloud);


	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);
	pt.put<std::string>("��������color.���뷧ֵ", distanceThreshold);
	pt.put<std::string>("��������color.��ɫ�ֵ", pointColorThreshold);
	pt.put<std::string>("��������color.��ɫ�ֵ", regionColorThreshold);
	pt.put<std::string>("��������color.��С��������", minClusterSize);
    boost::property_tree::ini_parser::write_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  

	ins->UpdateView();
	this->accept();
}

void QQuyuzengzhangColor::Fun(PointCloudT::Ptr cloud, PointCloudT::Ptr colorCloud /*= nullptr*/)
{
	if (!cloud)
	{
		cloud = PCManage::ins().cloud_;
	}

	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  
	float       distanceThreshold = pt.get<float>("��������color.���뷧ֵ");
	float       pointColorThreshold = pt.get<float>("��������color.��ɫ�ֵ");
	float       regionColorThreshold = pt.get<float>("��������color.��ɫ�ֵ");
	float       minClusterSize = pt.get<float>("��������color.��С��������");

	pcl::search::Search<PointT>::Ptr tree(new pcl::search::KdTree<PointT>);
	//	pcl::IndicesPtr indices(new std::vector <int>);
	pcl::RegionGrowingRGB<PointT> reg;//����������ɫ�����������ָ���Ķ���
	reg.setInputCloud(cloud);//���÷ָ�ԭʼ����
	//	reg.setIndices(indices);//���÷ָ������������
	reg.setSearchMethod(tree);//�����������������������
	reg.setDistanceThreshold(/*10*/distanceThreshold);//���þ�����ֵ��С�ڸ�ֵ����Ϊ�����
	reg.setPointColorThreshold(/*8*/pointColorThreshold);//���õ�֮���ɫ����ֵ��С�ڸ�ֵ����Ϊһ������
	reg.setRegionColorThreshold(/*15*/regionColorThreshold);//���þ���֮���ɫ����ֵ��С�ڸ�ֵ��Ӧ�úϲ��㷨���ϲ�Ϊͬһ������
	reg.setMinClusterSize(/*200*/minClusterSize);//���þ����е���������ޣ�������������ڸ�ֵ��Ӧ�úϲ��㷨���ϲ�������ٵ�һ������

// 	std::vector <pcl::PointIndices> clusters;
// 	reg.extract(clusters);//Ӧ�÷ָ��㷨����ȡ����
	

	reg.extract(PCManage::ins().jlClusters_);

    if (colorCloud)
    {
        PointCloudT::Ptr colored_cloud = reg.getColoredCloud();//�Ծ��������һ��ɫ����ͬ����ɫΪ��ͬ�ľ���
        pcl::copyPointCloud(*colored_cloud, *colorCloud);
    }
//     else
//     {
//         PointCloudT::Ptr colored_cloud = reg.getColoredCloud();//�Ծ��������һ��ɫ����ͬ����ɫΪ��ͬ�ľ���
//         pcl::copyPointCloud(*colored_cloud, *cloud);
//     }
	
}