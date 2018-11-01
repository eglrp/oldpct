#include "QGround.h"
#include <pcl/filters/extract_indices.h>
#include <pcl/segmentation/progressive_morphological_filter.h>
#include "QRendView.h"
#include "PCManage.h"
#include <pcl/common/common.h>
#include "dvprint.h"

QGround::QGround(QWidget *parent)
	: QSubDialogBase(parent)
{
	ui.setupUi(this);

	ui.min_lineEdit_2->setValidator(new QIntValidator(this));
	ui.max_lineEdit_2->setValidator(new QDoubleValidator(this));
	ui.max_lineEdit->setValidator(new QDoubleValidator(this));
    ui.max_lineEdit_3->setValidator(new QDoubleValidator(this));

	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  
	ui.min_lineEdit_2->setText(QString::fromLocal8Bit(pt.get<std::string>("�������.���ڴ�С").c_str()));
	ui.max_lineEdit_2->setText(QString::fromLocal8Bit(pt.get<std::string>("�������.б��ֵ").c_str()));
    ui.max_lineEdit_3->setText(QString::fromLocal8Bit(pt.get<std::string>("�������.��С�߶�").c_str()));
	ui.max_lineEdit->setText(QString::fromLocal8Bit(  pt.get<std::string>("�������.���߶�").c_str()));
	ui.radioButton->setChecked(pt.get<bool>("�������.�ų�"));
	ui.checkBox->setChecked(   pt.get<bool>("�������.�Ƿ�ʹ�ø߶�"));
}

QGround::~QGround()
{
}

void QGround::OnApply()
{
	QRendView* ins = QRendView::MainRendView();
	PointCloudT::Ptr cloud = PCManage::ins().cloud_;
	PointT minPt, maxPt;

	std::string windowsize = ui.min_lineEdit_2->text().toLocal8Bit().data();
	std::string slope = ui.max_lineEdit_2->text().toLocal8Bit().data();
	std::string maxlDistance = ui.max_lineEdit->text().toLocal8Bit().data();
    std::string minlDistance = ui.max_lineEdit_3->text().toLocal8Bit().data();
	bool checked = ui.radioButton->isChecked();
	bool useHeight = ui.checkBox->isChecked();

	pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_filtered(new pcl::PointCloud<pcl::PointXYZRGB>);
	pcl::PointIndicesPtr ground(new pcl::PointIndices);

	//	������̬�˲���
	pcl::ProgressiveMorphologicalFilter<PointT> pmf;
	pmf.setInputCloud(cloud);
	//���ô��Ĵ�С
	pmf.setMaxWindowSize(atoi(windowsize.c_str()));
	// ���ü���߶���ֵ��б��ֵ
	pmf.setSlope(atof(slope.c_str()));

	if (useHeight)
	{
// 		// ���ó�ʼ�߶Ȳ�������Ϊ�ǵ����
		pmf.setInitialDistance(atof(minlDistance.c_str()));

		// ���ñ���Ϊ�ǵ��������߶�
        pmf.setMaxDistance(atof(maxlDistance.c_str()));
	}


	

	//��ȡ����
	pmf.extract(ground->indices);

	// �ӱ�ŵ�����
	pcl::ExtractIndices<PointT> extract;
	extract.setInputCloud(cloud);
	extract.setIndices(ground);
	// ��ȡ�ǵ����
	extract.setNegative(checked);
	extract.filter(*cloud);




	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);
	pt.put<std::string>("�������.���ڴ�С", windowsize);  // д�ֶ�  
	pt.put<std::string>("�������.б��ֵ", slope);  // д�ֶ�  
	pt.put<std::string>("�������.���߶�", maxlDistance);  // д�ֶ�  
    pt.put<std::string>("�������.��С�߶�", minlDistance);  // д�ֶ�  
	pt.put<bool>("�������.�ų�", checked);  // д�ֶ�  
	pt.put<bool>("�������.�Ƿ�ʹ�ø߶�", useHeight);  // д�ֶ�  
    boost::property_tree::ini_parser::write_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  


	ins->UpdateView();
	this->accept();
}

void QGround::Fun(PointCloudT::Ptr cloud, PointCloudT &ground)
{
	if (!cloud)
		cloud = PCManage::ins().cloud_;

	PointT minPt, maxPt;


	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  
	std::string windowsize = pt.get<std::string>("�������.���ڴ�С");
	std::string slope = pt.get<std::string>("�������.б��ֵ");
	std::string maxlDistance = pt.get<std::string>("�������.���߶�");
    std::string minlDistance = pt.get<std::string>("�������.��С�߶�");
	//bool checked = pt.get<bool>("�������.�ų�");
	bool useHeight = pt.get<bool>("�������.�Ƿ�ʹ�ø߶�");

	pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_filtered(new pcl::PointCloud<pcl::PointXYZRGB>);
	pcl::PointIndicesPtr groundindices(new pcl::PointIndices);

	//	������̬�˲���
	pcl::ProgressiveMorphologicalFilter<PointT> pmf;
	pmf.setInputCloud(cloud);
	//���ô��Ĵ�С
	pmf.setMaxWindowSize(atoi(windowsize.c_str()));
	// ���ü���߶���ֵ��б��ֵ
	pmf.setSlope(atof(slope.c_str()));

	if (useHeight)
	{

		// ���ó�ʼ�߶Ȳ�������Ϊ�ǵ����
		pmf.setInitialDistance(atof(minlDistance.c_str()));

		// ���ñ���Ϊ�ǵ��������߶�
        pmf.setMaxDistance(atof(maxlDistance.c_str()));
	}

	//��ȡ����
    pmf.extract(groundindices->indices);

	// �ӱ�ŵ�����
	pcl::ExtractIndices<PointT> extract;
	extract.setInputCloud(cloud);
    extract.setIndices(groundindices);
    extract.filter(ground);
	// ��ȡ�ǵ����
	extract.setNegative(true);
	extract.filter(*cloud);

    //dd("��������=%d", ground->size());
}

// �����ƴ����ά����
void PutPointCloud2Arr(PointCloudT &cloud, std::vector<std::vector<pcl::PointIndices>> &pointsArr, int row, int col, PointT min, PointT max, double gridSize)
{
    // �������
    for (int i = 0; i < cloud.size(); ++i)
    {
        PointT &pt = cloud.at(i);
        pointsArr[(int)((pt.x - min.x) / gridSize)][(int)((pt.y - min.y) / gridSize)].indices.push_back(i);
    }
}

 