#include "QChaotiJulei.h"
#include <pcl/segmentation/supervoxel_clustering.h>
#include "QRendView.h"
#include "PCManage.h"

QChaotiJulei::QChaotiJulei(QWidget *parent)
	: QSubDialogBase(parent)
{
	ui.setupUi(this);
	ui.lineEdit->setValidator(new QDoubleValidator(this));
	ui.lineEdit_2->setValidator(new QDoubleValidator(this));
	ui.lineEdit_3->setValidator(new QDoubleValidator(this));
	ui.lineEdit_4->setValidator(new QDoubleValidator(this));
	ui.lineEdit_5->setValidator(new QDoubleValidator(this));

	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  
	ui.lineEdit->setText(QString::fromLocal8Bit(pt.get<std::string>("�������.���طֱ���").c_str()));
	ui.lineEdit_2->setText(QString::fromLocal8Bit(pt.get<std::string>("�������.���ӷֱ���").c_str()));
	ui.lineEdit_3->setText(QString::fromLocal8Bit(pt.get<std::string>("�������.��ɫȨ��").c_str()));
	ui.lineEdit_4->setText(QString::fromLocal8Bit(pt.get<std::string>("�������.�ռ�Ȩ��").c_str()));
	ui.lineEdit_5->setText(QString::fromLocal8Bit(pt.get<std::string>("�������.����Ȩ��").c_str()));
}

QChaotiJulei::~QChaotiJulei()
{
}

void QChaotiJulei::OnApply()
{
	QRendView* ins = QRendView::MainRendView();
	PointCloudT::Ptr cloud = PCManage::ins().cloud_;


	//�趨�ᾧ����
	float voxel_resolution =/* 0.008f*/atof(ui.lineEdit->text().toLocal8Bit().data());
	float seed_resolution = /*0.1f*/atof(ui.lineEdit_2->text().toLocal8Bit().data());
	float color_importance = /*0.2f*/atof(ui.lineEdit_3->text().toLocal8Bit().data());
	float spatial_importance = /*0.4f*/atof(ui.lineEdit_4->text().toLocal8Bit().data());
	float normal_importance = /*1.0f*/atof(ui.lineEdit_5->text().toLocal8Bit().data());
	bool disable_transform = true;

	//���ɽᾧ��
	pcl::SupervoxelClustering<PointT> super(voxel_resolution, seed_resolution);
	//�͵�����ʽ�й�
	if (disable_transform)
		super.setUseSingleCameraTransform(false);
	//������Ƽ��ᾧ����
	super.setInputCloud(cloud);
	super.setColorImportance(color_importance);
	super.setSpatialImportance(spatial_importance);
	super.setNormalImportance(normal_importance);
	//����ᾧ�ָ����������һ��ӳ���
	std::map <uint32_t, pcl::Supervoxel<PointT>::Ptr > supervoxel_clusters;
	super.extract(supervoxel_clusters);
	//��þ������� 
	PointCloudT::Ptr voxel_centroid_cloud = super.getVoxelCentroidCloud(); //��þ���   
	pcl::PointCloud<pcl::PointXYZL>::Ptr labeled_voxel_cloud = super.getLabeledVoxelCloud();

	PointCloudT::Ptr add_cloud = boost::make_shared<PointCloudT>();
	std::multimap<uint32_t, uint32_t> supervoxel_adjacency;
	super.getSupervoxelAdjacency(supervoxel_adjacency);  std::multimap<uint32_t, uint32_t>::iterator label_itr = supervoxel_adjacency.begin();
	for (; label_itr != supervoxel_adjacency.end();)
	{
		//First get the label
		uint32_t supervoxel_label = label_itr->first;
		//Now get the supervoxel corresponding to the label
		pcl::Supervoxel<PointT>::Ptr supervoxel = supervoxel_clusters.at(supervoxel_label);
		//Now we need to iterate through the adjacent supervoxels and make a point cloud of them
		PointCloudT adjacent_supervoxel_centers;
		std::multimap<uint32_t, uint32_t>::iterator adjacent_itr = supervoxel_adjacency.equal_range(supervoxel_label).first;
		for (; adjacent_itr != supervoxel_adjacency.equal_range(supervoxel_label).second; ++adjacent_itr)
		{
			pcl::Supervoxel<PointT>::Ptr neighbor_supervoxel = supervoxel_clusters.at(adjacent_itr->second);
			PointT t;
			t.x = neighbor_supervoxel->centroid_.x;
			t.y = neighbor_supervoxel->centroid_.y;
			t.z = neighbor_supervoxel->centroid_.z;
			t.r = neighbor_supervoxel->centroid_.r;
			t.g = neighbor_supervoxel->centroid_.g;
			t.b = neighbor_supervoxel->centroid_.b;
			adjacent_supervoxel_centers.push_back(t);
		}
		//Now we make a name for this polygon
		std::stringstream ss;
		ss << "supervoxel_" << supervoxel_label;
		//This function is shown below, but is beyond the scope of this tutorial - basically it just generates a "star" polygon mesh from the points given
		//addSupervoxelConnectionsToViewer(supervoxel->centroid_, adjacent_supervoxel_centers, ss.str(), viewer);

		uint8_t r = rand() % 256;
		uint8_t g = rand() % 256;
		uint8_t b = rand() % 256;
		for (int i = 0; i < adjacent_supervoxel_centers.size(); ++i)
		{
			adjacent_supervoxel_centers.points[i].r = r;
			adjacent_supervoxel_centers.points[i].g = g;
			adjacent_supervoxel_centers.points[i].b = b;
		}

		*add_cloud += adjacent_supervoxel_centers;
		//Move iterator forward to next label
		label_itr = supervoxel_adjacency.upper_bound(supervoxel_label);
	}
	pcl::copyPointCloud(*add_cloud, *cloud);




	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);
	pt.put<float>("�������.���طֱ���", voxel_resolution);
	pt.put<float>("�������.���ӷֱ���", seed_resolution);
	pt.put<float>("�������.��ɫȨ��", color_importance);
	pt.put<float>("�������.�ռ�Ȩ��", spatial_importance);
	pt.put<float>("�������.����Ȩ��", normal_importance);
    boost::property_tree::ini_parser::write_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  


	ins->UpdateView();
	this->accept();
}
