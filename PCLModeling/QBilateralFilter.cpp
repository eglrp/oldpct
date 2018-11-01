#include "QBilateralFilter.h"
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/search/kdtree.h>
#include <pcl/filters/fast_bilateral.h>
#include "QRendView.h"
#include "PCManage.h"

QBilateralFilter::QBilateralFilter(QWidget *parent)
	: QSubDialogBase(parent)
{
	ui.setupUi(this);

	ui.min_lineEdit->setValidator(new QDoubleValidator(this));
	ui.max_lineEdit->setValidator(new QDoubleValidator(this));

	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  
	ui.min_lineEdit->setText(QString::fromLocal8Bit(pt.get<std::string>("˫�߹���.���ڴ�С").c_str()));
	ui.max_lineEdit->setText(QString::fromLocal8Bit(pt.get<std::string>("˫�߹���.��׼ƫ��").c_str()));
}

QBilateralFilter::~QBilateralFilter()
{
}

void QBilateralFilter::OnApply()
{
	QRendView* ins = QRendView::MainRendView();
	PointCloudT::Ptr cloud = PCManage::ins().cloud_;

	std::string windowsize = ui.min_lineEdit->text().toLocal8Bit().data();
	std::string standardPiancha = ui.max_lineEdit->text().toLocal8Bit().data();

	double sigma_s_ = atof(windowsize.c_str());//5.0
	double sigma_r_ = atof(standardPiancha.c_str());//0.03


	if (sigma_s_ <= 1.0f)
		sigma_s_ = 1.0f;
	if (sigma_r_ <= 0.0001f)
		sigma_r_ = 0.0001f;

	pcl::FastBilateralFilter<PointT> fbf;
	fbf.setInputCloud(cloud);
	fbf.setSigmaS(sigma_s_);
	fbf.setSigmaR(sigma_r_);
	fbf.filter(*cloud);


	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);
	pt.put<std::string>("˫�߹���.���ڴ�С", windowsize);  // д�ֶ�  
	pt.put<std::string>("˫�߹���.��׼ƫ��", standardPiancha);  // д�ֶ�  
    boost::property_tree::ini_parser::write_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  

	ins->UpdateView();
	this->accept();
}
