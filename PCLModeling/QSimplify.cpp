#include "QSimplify.h"
#include <QRendView.h>
#include <pcl/filters/uniform_sampling.h>   //���Ȳ���
#include "PCManage.h"

QSimplify::QSimplify(QWidget *parent)
	: QSubDialogBase(parent)
{
	ui.setupUi(this);
	ui.lineEdit->setValidator(new QDoubleValidator(this));

	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  
	ui.lineEdit->setText(QString::fromLocal8Bit(pt.get<std::string>("���Ƽ�.�����С").c_str()));
}

QSimplify::~QSimplify()
{
}

void QSimplify::OnApply()
{
	QRendView* ins = QRendView::MainRendView();
	PointCloudT::Ptr cloud = PCManage::ins().cloud_;

	std::string   gridSize = ui.lineEdit->text().toLocal8Bit().data();

    double leaf = atof(gridSize.c_str());

    //���Ȳ������Ʋ���ȡ�ؼ���      �����²��������Ĵ���
    pcl::UniformSampling<pcl::PointXYZRGB> uniform_sampling;
    uniform_sampling.setInputCloud(cloud);  //�������
    uniform_sampling.setRadiusSearch(leaf);   //���ð뾶 
    uniform_sampling.filter(*cloud);   //�˲�

	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);
	pt.put<std::string>("���Ƽ�.�����С", gridSize);
    boost::property_tree::ini_parser::write_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  


	ins->UpdateView();
	this->accept();
}

void QSimplify::Fun(PointCloudT::Ptr cloud)
{
	if (!cloud)
		cloud = PCManage::ins().cloud_;

	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  
	std::string   gridSize = pt.get<std::string>("���Ƽ�.�����С");


    double leaf = atof(gridSize.c_str());

    //���Ȳ������Ʋ���ȡ�ؼ���      �����²��������Ĵ���
    pcl::UniformSampling<pcl::PointXYZRGB> uniform_sampling;
    uniform_sampling.setInputCloud(cloud);  //�������
    uniform_sampling.setRadiusSearch(leaf);   //���ð뾶 
    uniform_sampling.filter(*cloud);   //�˲�
}
