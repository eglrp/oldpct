#include "QZhiTongXYZ.h"
#include <QDoubleValidator>
#include <QRendView.h>
#include <pcl/filters/passthrough.h>
#include <pcl/common/common.h>
#include <PCManage.h>

QZhiTongXYZ::QZhiTongXYZ(QWidget *parent)
: QSubDialogBase(parent)
{
	ui.setupUi(this);
	ui.comboBox->addItem(QStringLiteral("x"));
	ui.comboBox->addItem(QStringLiteral("y"));
	ui.comboBox->addItem(QStringLiteral("z"));
	
	ui.min_lineEdit->setValidator(new QDoubleValidator(this));
	ui.max_lineEdit->setValidator(new QDoubleValidator(this));

	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  
	ui.comboBox->setCurrentText(QString::fromLocal8Bit(pt.get<std::string>("ֱͨXYZ.XYZ").c_str()));
	ui.min_lineEdit->setText(QString::fromLocal8Bit(pt.get<std::string>("ֱͨXYZ.��Сֵ").c_str()));
	ui.max_lineEdit->setText(QString::fromLocal8Bit(pt.get<std::string>("ֱͨXYZ.���ֵ").c_str()));
	ui.radioButton->setChecked(pt.get<bool>("ֱͨXYZ.�ų�"));
}

QZhiTongXYZ::~QZhiTongXYZ()
{
}

void QZhiTongXYZ::OnApply()
{
	QRendView* ins = QRendView::MainRendView();
	PointCloudT::Ptr cloud = PCManage::ins().cloud_;
	PointT minPt, maxPt;

	std::string xyz = ui.comboBox->currentText().toLocal8Bit().data();
	std::string min = ui.min_lineEdit->text().toLocal8Bit().data();
	std::string max = ui.max_lineEdit->text().toLocal8Bit().data();
	bool checked = ui.radioButton->isChecked();

	// Create the filtering object  
	pcl::PassThrough<PointT> pass;
	pass.setInputCloud(cloud);
	pass.setFilterFieldName(xyz);
	pass.setFilterLimits(atof(min.c_str()), atof(max.c_str()));
	pass.setFilterLimitsNegative(checked);	//�����true���ų�
	pass.filter(*cloud);

	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);
	pt.put<std::string>("ֱͨXYZ.XYZ", xyz);  // д�ֶ�  
	pt.put<std::string>("ֱͨXYZ.��Сֵ", min);  // д�ֶ�  
	pt.put<std::string>("ֱͨXYZ.���ֵ", max);  // д�ֶ�  
	pt.put<bool>("ֱͨXYZ.�ų�", checked);  // д�ֶ�  
    boost::property_tree::ini_parser::write_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  
	

	ins->UpdateView();

	this->accept();
}
