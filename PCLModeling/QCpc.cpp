#include "QCpc.h"
#include <pcl/segmentation/supervoxel_clustering.h>
#include <pcl/segmentation/cpc_segmentation.h>
#include "QRendView.h"
#include "PCManage.h"

#define Random(x) (rand() % x)  
QCpc::QCpc(QWidget *parent)
	: QSubDialogBase(parent)
{
	ui.setupUi(this);

	ui.min_lineEdit_23->setValidator(new QDoubleValidator(this));
	ui.max_lineEdit_23->setValidator(new QDoubleValidator(this));
	ui.min_lineEdit_24->setValidator(new QDoubleValidator(this));
	ui.max_lineEdit_24->setValidator(new QIntValidator(this));
	ui.min_lineEdit_22->setValidator(new QIntValidator(this));
	ui.max_lineEdit_22->setValidator(new QDoubleValidator(this));
	ui.min_lineEdit_21->setValidator(new QDoubleValidator(this));
	ui.max_lineEdit_21->setValidator(new QDoubleValidator(this));

	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  
	ui.min_lineEdit_23->setText(QString::fromLocal8Bit(pt.get<std::string>("cpc.���Ӿ���").c_str()));
	ui.max_lineEdit_23->setText(QString::fromLocal8Bit(pt.get<std::string>("cpc.���˾���").c_str()));
	ui.min_lineEdit_24->setText(QString::fromLocal8Bit(pt.get<std::string>("cpc.��ɫ�ݲ�").c_str()));
	ui.max_lineEdit_24->setText(QString::fromLocal8Bit(pt.get<std::string>("cpc.�������������").c_str()));
	ui.min_lineEdit_22->setText(QString::fromLocal8Bit(pt.get<std::string>("cpc.��С�и�����").c_str()));
	ui.max_lineEdit_22->setText(QString::fromLocal8Bit(pt.get<std::string>("cpc.�����и����С��������ﵽ��ִ�е�Ŀ��").c_str()));
	ui.min_lineEdit_21->setText(QString::fromLocal8Bit(pt.get<std::string>("cpc.�ռ�Ȩ��").c_str()));
	ui.max_lineEdit_21->setText(QString::fromLocal8Bit(pt.get<std::string>("cpc.����Ȩ��").c_str()));
}

QCpc::~QCpc()
{
}
void QCpc::OnApply()
{
	QRendView* ins = QRendView::MainRendView();
	PointCloudT::Ptr cloud = PCManage::ins().cloud_;

	//�������  
	PointCloudT::Ptr input_cloud_ptr(new pcl::PointCloud<PointT>);
	pcl::copyPointCloud(*cloud, *input_cloud_ptr);

	//������� �������������Ӿ��롢���˾��롢��ɫ�ݲ  
	std::string       voxel_resolution = ui.min_lineEdit_23->text().toLocal8Bit().data();
	std::string       seed_resolution = ui.max_lineEdit_23->text().toLocal8Bit().data();
	std::string       color_importance = ui.min_lineEdit_24->text().toLocal8Bit().data();
	std::string       maxcut = ui.max_lineEdit_24->text().toLocal8Bit().data();
	std::string       mincut = ui.min_lineEdit_22->text().toLocal8Bit().data();
	std::string		minscore = ui.max_lineEdit_22->text().toLocal8Bit().data();
	std::string   spatial_importance = ui.min_lineEdit_21->text().toLocal8Bit().data();
	std::string   normal_importance = ui.max_lineEdit_21->text().toLocal8Bit().data();

	bool use_single_cam_transform = false;
	bool use_supervoxel_refinement = false;

	unsigned int k_factor = 0;
	
	
	//voxel_resolution is the resolution (in meters) of voxels used��seed_resolution is the average size (in meters) of resulting supervoxels    
	pcl::SupervoxelClustering<PointT> super(atof(voxel_resolution.c_str()), atof(seed_resolution.c_str()));
	super.setUseSingleCameraTransform(use_single_cam_transform);
	super.setInputCloud(input_cloud_ptr);
	//Set the importance of color for supervoxels.   
	super.setColorImportance(atof(color_importance.c_str()));
	//Set the importance of spatial distance for supervoxels.  
	super.setSpatialImportance(atof(spatial_importance.c_str()));
	//Set the importance of scalar normal product for supervoxels.   
	super.setNormalImportance(atof(normal_importance.c_str()));
	std::map<uint32_t, pcl::Supervoxel<PointT>::Ptr> supervoxel_clusters;

	super.extract(supervoxel_clusters);

	std::multimap<uint32_t, uint32_t> supervoxel_adjacency;
	super.getSupervoxelAdjacency(supervoxel_adjacency);
	pcl::PointCloud<pcl::PointNormal>::Ptr sv_centroid_normal_cloud = pcl::SupervoxelClustering<PointT>::makeSupervoxelNormalCloud(supervoxel_clusters);


	//����CPC�ָ���
	pcl::CPCSegmentation<PointT> seg;
	//���볬�������
	seg.setInputSupervoxels(supervoxel_clusters, supervoxel_adjacency);
	//���÷ָ����
	seg.setCutting(atoi(maxcut.c_str()),
		atoi(mincut.c_str()),
		atof(minscore.c_str()),
		true,
		true,
		false);
	unsigned int ransac_iterations = 10;
	seg.setRANSACIterations(ransac_iterations);
	seg.segment();
	pcl::PointCloud<pcl::PointXYZL>::Ptr  labeled_cloud_arg = boost::make_shared<pcl::PointCloud<pcl::PointXYZL>>();
	seg.relabelCloud(*labeled_cloud_arg);


	ofstream outFile2("�ָ��ϲ�3.txt", std::ios_base::out);
	for (int i = 0; i < labeled_cloud_arg->size(); i++) {
		outFile2 << labeled_cloud_arg->points[i].x << "\t" << labeled_cloud_arg->points[i].y << "\t" << labeled_cloud_arg->points[i].z << "\t" << labeled_cloud_arg->points[i].label << endl;
	}

	int label_max2 = 0;
	for (int i = 0; i< labeled_cloud_arg->size(); i++) {
		if (labeled_cloud_arg->points[i].label>label_max2)
			label_max2 = labeled_cloud_arg->points[i].label;
	}
	PointCloudT::Ptr ColoredCloud2(new PointCloudT);
	ColoredCloud2->height = 1;
	ColoredCloud2->width = labeled_cloud_arg->size();
	ColoredCloud2->resize(labeled_cloud_arg->size());
	for (int i = 0; i < label_max2; i++) {
		int color_R = Random(255);
		int color_G = Random(255);
		int color_B = Random(255);

		for (int j = 0; j < labeled_cloud_arg->size(); j++) {
			if (labeled_cloud_arg->points[j].label == i) {
				ColoredCloud2->points[j].x = labeled_cloud_arg->points[j].x;
				ColoredCloud2->points[j].y = labeled_cloud_arg->points[j].y;
				ColoredCloud2->points[j].z = labeled_cloud_arg->points[j].z;
				ColoredCloud2->points[j].r = color_R;
				ColoredCloud2->points[j].g = color_G;
				ColoredCloud2->points[j].b = color_B;
			}
		}
	}
	//	pcl::io::savePCDFileASCII("�ָ��ϲ�3.pcd", *ColoredCloud2);
	pcl::copyPointCloud(*ColoredCloud2, *cloud);


	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);
	pt.put<std::string>("cpc.���Ӿ���", voxel_resolution);
	pt.put<std::string>("cpc.���˾���", seed_resolution);
	pt.put<std::string>("cpc.��ɫ�ݲ�", color_importance);
	pt.put<std::string>("cpc.�������������", maxcut);
	pt.put<std::string>("cpc.��С�и�����", mincut);
	pt.put<std::string>("cpc.�����и����С��������ﵽ��ִ�е�Ŀ��", minscore);
	pt.put<std::string>("cpc.�ռ�Ȩ��", spatial_importance);
	pt.put<std::string>("cpc.����Ȩ��", normal_importance);


    boost::property_tree::ini_parser::write_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  


	ins->UpdateView();
	this->accept();
}
