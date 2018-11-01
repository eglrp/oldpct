#include "QRanSac.h"
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/filters/extract_indices.h>
#include "QRendView.h"
#include "PCManage.h"
#include <pcl/search/kdtree.h>

QRanSac::QRanSac(QWidget *parent)
: QSubDialogBase(parent)
{
	ui.setupUi(this);
	cloud_ = boost::make_shared<PointCloudT>();

	ui.min_lineEdit->setValidator(new QIntValidator(this));
	ui.max_lineEdit->setValidator(new QDoubleValidator(this));
	ui.max_lineEdit_2->setValidator(new QDoubleValidator(this));
	QStringList items;
	items << QStringLiteral("SACMODEL_PLANE")
		<< QStringLiteral("SACMODEL_LINE")
		<< QStringLiteral("SACMODEL_CIRCLE2D")
		<< QStringLiteral("SACMODEL_CIRCLE3D")
		<< QStringLiteral("SACMODEL_SPHERE")
		<< QStringLiteral("SACMODEL_CYLINDER")
		<< QStringLiteral("SACMODEL_CONE")
		<< QStringLiteral("SACMODEL_TORUS")
		<< QStringLiteral("SACMODEL_PARALLEL_LINE")
		<< QStringLiteral("SACMODEL_PERPENDICULAR_PLANE")
		<< QStringLiteral("SACMODEL_PARALLEL_LINES")
		<< QStringLiteral("SACMODEL_NORMAL_PLANE")
		<< QStringLiteral("SACMODEL_NORMAL_SPHERE")
		<< QStringLiteral("SACMODEL_REGISTRATION")
		<< QStringLiteral("SACMODEL_REGISTRATION_2D")
		<< QStringLiteral("SACMODEL_PARALLEL_PLANE")
		<< QStringLiteral("SACMODEL_NORMAL_PARALLEL_PLANE")
		<< QStringLiteral("SACMODEL_STICK");
	ui.comboBox->addItems(items);
	
	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  
	ui.min_lineEdit->setText(QString::fromLocal8Bit(pt.get<std::string>("ransac.����������").c_str()));
	ui.max_lineEdit->setText(QString::fromLocal8Bit(pt.get<std::string>("ransac.��ѯ�㵽Ŀ��ģ�͵ľ�����ֵ").c_str()));
	ui.max_lineEdit_2->setText(QString::fromLocal8Bit(pt.get<std::string>("ransac.����һ��������������Ⱥ��ĸ���").c_str()));
	ui.comboBox->setCurrentIndex(pt.get<int>("ransac.ͼԪ����"));
}

QRanSac::~QRanSac()
{
}


void QRanSac::OnApply()
{
	QRendView* ins = QRendView::MainRendView();
	PointCloudT::Ptr cloud = PCManage::ins().cloud_;

	int curindex = ui.comboBox->currentIndex();
	int maxIterations = atoi(ui.min_lineEdit->text().toLocal8Bit().data());
	double distanceThreshold = atof(ui.max_lineEdit->text().toLocal8Bit().data());
	double probability = atof(ui.max_lineEdit_2->text().toLocal8Bit().data());

	
	//����һ��ģ�Ͳ����������ڼ�¼���
	pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients);
	//inliers��ʾ��������̵ĵ� ��¼���ǵ��Ƶ����
	pcl::PointIndices::Ptr inliers(new pcl::PointIndices);
	// ����һ���ָ���
	pcl::SACSegmentation<PointT> seg;
	// Optional
	seg.setOptimizeCoefficients(true);
	// Mandatory-����Ŀ�꼸����״
	seg.setModelType(/*pcl::SACMODEL_CIRCLE3D*/curindex);
	//�ָ�������������
	seg.setMethodType(pcl::SAC_RANSAC);
	//������������Ĭ��ֵΪ50��
	seg.setMaxIterations(maxIterations);
	//��ѯ�㵽Ŀ��ģ�͵ľ�����ֵ
	seg.setDistanceThreshold(distanceThreshold);
	//����һ��������������Ⱥ��ĸ��ʣ�Ĭ��ֵΪ0.99����
//	seg.setProbability(probability);
// 	seg.setAxis(Eigen::Vector3f(0, 1, 0));
// 	seg.setEpsAngle(10.0f * (M_PI / 180.0f));//����Լ��
// 	pcl::SACSegmentation<PointT>::SearchPtr searcher(new pcl::search::KdTree<PointT>);
// 	seg.setSamplesMaxDist(3, searcher);
//	seg.setRadiusLimits(1000, (std::numeric_limits<double>::max)());
	//�������
	seg.setInputCloud(cloud);
	//�ָ����
	seg.segment(*inliers, *coefficients);
	//pcl::copyPointCloud(*cloud, *inliers, *cloud);


	PointCloudT tempCloud;
	pcl::ExtractIndices<PointT> extract;
	extract.setInputCloud(cloud);
	extract.setIndices(inliers);
	extract.filter(tempCloud);
	//pcl::copyPointCloud(tempCloud, *ui.widget->cloud_);
	*cloud_ += tempCloud;
	// ��ȡ�ǵ����
	extract.setNegative(true);
	extract.filter(*cloud);

	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);
	pt.put<int>("ransac.����������", maxIterations);
	pt.put<double>("ransac.��ѯ�㵽Ŀ��ģ�͵ľ�����ֵ", distanceThreshold);
	pt.put<double>("ransac.����һ��������������Ⱥ��ĸ���", probability);
	pt.put<int>("ransac.ͼԪ����", curindex);



    boost::property_tree::ini_parser::write_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  

	ui.widget->UpdateView(cloud_);
	ins->UpdateView();
	//this->accept();
}
