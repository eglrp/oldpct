#include "QOushifenge.h"
#include <pcl/filters/voxel_grid.h>
#include <pcl/io/pcd_io.h>
#include <pcl/search/kdtree.h>
#include <pcl/segmentation/extract_clusters.h>
#include "QRendView.h"
#include "PCManage.h"

QOushifenge::QOushifenge(QWidget *parent)
	: QSubDialogBase(parent)
{
	ui.setupUi(this);
	ui.lineEdit->setValidator(new QDoubleValidator(this));
	ui.lineEdit_2->setValidator(new QIntValidator(this));
	ui.lineEdit_3->setValidator(new QIntValidator(this));

	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  
	ui.lineEdit->setText(QString::fromLocal8Bit(  pt.get<std::string>("ŷ�Ϸָ�.�ٽ��뾶").c_str()));
	ui.lineEdit_2->setText(QString::fromLocal8Bit(pt.get<std::string>("ŷ�Ϸָ�.��С�������Ŀ").c_str()));
	ui.lineEdit_3->setText(QString::fromLocal8Bit(pt.get<std::string>("ŷ�Ϸָ�.���������Ŀ").c_str()));
}

QOushifenge::~QOushifenge()
{
}

void QOushifenge::OnApply()
{
	QRendView* ins = QRendView::MainRendView();
	PointCloudT::Ptr cloud = PCManage::ins().cloud_;

	std::string       clusterTolerance = ui.lineEdit->text().toLocal8Bit().data();
	std::string       minClusterSize = ui.lineEdit_2->text().toLocal8Bit().data();
	std::string       maxClusterSize = ui.lineEdit_3->text().toLocal8Bit().data();

	

	// �xȡ�ļ�
	PointCloudT::Ptr add_cloud(new PointCloudT);

	// �²ɘӣ��w���~�Ӵ�С��0.01
	pcl::VoxelGrid<PointT> vg;
	PointCloudT::Ptr cloud_filtered(new PointCloudT);
	vg.setInputCloud(cloud);
	vg.setLeafSize(0.01f, 0.01f, 0.01f);
	vg.filter(*cloud_filtered);
	std::cout << "PointCloud after filtering has: " << cloud_filtered->points.size() << " data points." << std::endl; //*
	pcl::PCDWriter writer;


	// ���������ȡ�ь�������kdtree�����
	pcl::search::KdTree<PointT>::Ptr tree(new pcl::search::KdTree<PointT>);
	tree->setInputCloud(cloud_filtered);

	std::vector<pcl::PointIndices> cluster_indices;
	pcl::EuclideanClusterExtraction<PointT> ec;   //�Wʽ������
	//ec.setClusterTolerance(0.02);                     // �O�������ь����ь��돽��2cm
	ec.setClusterTolerance(/*2*/atof(clusterTolerance.c_str()));                     // �O�������ь����ь��돽��2cm
	ec.setMinClusterSize(atoi(/*100*/minClusterSize.c_str()));                 //�O��һ�������Ҫ�����ٵ��c��Ŀ��100
	ec.setMaxClusterSize(/*2500000*/atoi(maxClusterSize.c_str()));               //�O��һ�������Ҫ������c��Ŀ��25000
	ec.setSearchMethod(tree);                    //�O���c녵��ь��C��
	ec.setInputCloud(cloud_filtered);
	ec.extract(cluster_indices);           //���c�����ȡ����K���c�����������cluster_indices��
	PCManage::ins().jlClusters_ = cluster_indices;
	//�����L���c�����cluster_indices,ֱ���ָ�̎���о��
	int j = 0;
	for (std::vector<pcl::PointIndices>::const_iterator it = cluster_indices.begin(); it != cluster_indices.end(); ++it)
	{ //���������е��c녵��������K�ҷ��_�����������c�
		PointCloudT::Ptr cloud_cluster(new PointCloudT);
		for (std::vector<int>::const_iterator pit = it->indices.begin(); pit != it->indices.end(); ++pit)
			//�O�������c녵Č��Ԇ��}
			cloud_cluster->points.push_back(cloud_filtered->points[*pit]); //*
		cloud_cluster->width = cloud_cluster->points.size();
		cloud_cluster->height = 1;
		cloud_cluster->is_dense = true;
		// 		std::cout << "PointCloud representing the Cluster: " << cloud_cluster->points.size() << " data points." << std::endl;
		std::stringstream ss;
		ss << "cloud_cluster_" << j << ".pcd";
		writer.write<PointT>(ss.str(), *cloud_cluster, false); //*

		//���������������������������Ͼ��ǌ��F���еľ�Ĳ��E���K�҃����ˡ�������������������������������������������������������//
		//���¾����Ҟ��˻ش�W���ᆖ��Qҕ�X������ƽ�������ҕ�X���ĳ�ʽ�aҲ�̓���   
		uint8_t r = rand() % 256;
		uint8_t g = rand() % 256;
		uint8_t b = rand() % 256;
		for (int i = 0; i < cloud_cluster->size(); ++i)
		{
			cloud_cluster->points[i].r = r;
			cloud_cluster->points[i].g = g;
			cloud_cluster->points[i].b = b;
		}

		*add_cloud += *cloud_cluster;


		//pcl::io::savePCDFileASCII("add_cloud.pcd", *add_cloud);
	}
	pcl::copyPointCloud(*add_cloud, *cloud);



	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);
	pt.put<std::string>("ŷ�Ϸָ�.�ٽ��뾶", clusterTolerance);
	pt.put<std::string>("ŷ�Ϸָ�.��С�������Ŀ", minClusterSize);
	pt.put<std::string>("ŷ�Ϸָ�.���������Ŀ", maxClusterSize);
    boost::property_tree::ini_parser::write_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  


	ins->UpdateView();
	this->accept();
}

void QOushifenge::Fun(PointCloudT::Ptr cloud)
{
	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  
	std::string       clusterTolerance = pt.get<std::string>("ŷ�Ϸָ�.�ٽ��뾶");
	std::string       minClusterSize = pt.get<std::string>("ŷ�Ϸָ�.��С�������Ŀ");
	std::string       maxClusterSize = pt.get<std::string>("ŷ�Ϸָ�.���������Ŀ");



	// �xȡ�ļ�
	PointCloudT::Ptr add_cloud(new PointCloudT);

	// ���������ȡ�ь�������kdtree�����
	pcl::search::KdTree<PointT>::Ptr tree(new pcl::search::KdTree<PointT>);
    tree->setInputCloud(cloud);

	std::vector<pcl::PointIndices> cluster_indices;
	pcl::EuclideanClusterExtraction<PointT> ec;   //�Wʽ������
	ec.setClusterTolerance(/*2*/atof(clusterTolerance.c_str()));                     // �O�������ь����ь��돽��2cm
	ec.setMinClusterSize(atoi(/*100*/minClusterSize.c_str()));                 //�O��һ�������Ҫ�����ٵ��c��Ŀ��100
	ec.setMaxClusterSize(/*2500000*/atoi(maxClusterSize.c_str()));               //�O��һ�������Ҫ������c��Ŀ��25000
	ec.setSearchMethod(tree);                    //�O���c녵��ь��C��
    ec.setInputCloud(cloud);
	ec.extract(cluster_indices);           //���c�����ȡ����K���c�����������cluster_indices��


	PCManage::ins().jlClusters_ = cluster_indices;

    for (std::vector<pcl::PointIndices>::const_iterator it = cluster_indices.begin(); it != cluster_indices.end(); ++it)
    {
        uint8_t r = rand() % 256;
        uint8_t g = rand() % 256;
        uint8_t b = rand() % 256;
        for (std::vector<int>::const_iterator pit = it->indices.begin(); pit != it->indices.end(); ++pit)
        {
            cloud->at(*pit).r = r;
            cloud->at(*pit).g = g;
            cloud->at(*pit).b = b;
        }
    }
}

void QOushifenge::Fun(PointCloudT::Ptr cloud, std::vector<pcl::PointIndices>& cluster_indices)
{
    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  
    std::string       clusterTolerance = pt.get<std::string>("ŷ�Ϸָ�.�ٽ��뾶");
    std::string       minClusterSize = pt.get<std::string>("ŷ�Ϸָ�.��С�������Ŀ");
    std::string       maxClusterSize = pt.get<std::string>("ŷ�Ϸָ�.���������Ŀ");

    // �xȡ�ļ�
    PointCloudT::Ptr add_cloud(new PointCloudT);
    // ���������ȡ�ь�������kdtree�����
    pcl::search::KdTree<PointT>::Ptr tree(new pcl::search::KdTree<PointT>);
    tree->setInputCloud(cloud);

    pcl::EuclideanClusterExtraction<PointT> ec;   //�Wʽ������
    ec.setClusterTolerance(/*2*/atof(clusterTolerance.c_str()));                     // �O�������ь����ь��돽��2cm
    ec.setMinClusterSize(atoi(/*100*/minClusterSize.c_str()));                 //�O��һ�������Ҫ�����ٵ��c��Ŀ��100
    ec.setMaxClusterSize(/*2500000*/atoi(maxClusterSize.c_str()));               //�O��һ�������Ҫ������c��Ŀ��25000
    ec.setSearchMethod(tree);                    //�O���c녵��ь��C��
    ec.setInputCloud(cloud);
    ec.extract(cluster_indices);           //���c�����ȡ����K���c�����������cluster_indices��
}

void QOushifenge::Fun(PointCloudT::Ptr cloud, std::vector<pcl::PointIndices>& cluster_indices, double k)
{
    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  
    std::string       clusterTolerance = pt.get<std::string>("ŷ�Ϸָ�.�ٽ��뾶");
    std::string       minClusterSize = pt.get<std::string>("ŷ�Ϸָ�.��С�������Ŀ");
    std::string       maxClusterSize = pt.get<std::string>("ŷ�Ϸָ�.���������Ŀ");

    // �xȡ�ļ�
    PointCloudT::Ptr add_cloud(new PointCloudT);
    // ���������ȡ�ь�������kdtree�����
    pcl::search::KdTree<PointT>::Ptr tree(new pcl::search::KdTree<PointT>);
    tree->setInputCloud(cloud);

    pcl::EuclideanClusterExtraction<PointT> ec;   //�Wʽ������
    ec.setClusterTolerance(k);                     // �O�������ь����ь��돽��2cm
    ec.setMinClusterSize(atoi(/*100*/minClusterSize.c_str()));                 //�O��һ�������Ҫ�����ٵ��c��Ŀ��100
    ec.setMaxClusterSize(/*2500000*/atoi(maxClusterSize.c_str()));               //�O��һ�������Ҫ������c��Ŀ��25000
    ec.setSearchMethod(tree);                    //�O���c녵��ь��C��
    ec.setInputCloud(cloud);
    ec.extract(cluster_indices);           //���c�����ȡ����K���c�����������cluster_indices��
}
