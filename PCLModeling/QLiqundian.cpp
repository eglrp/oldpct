#include "QLiqundian.h"
#include <pcl/filters/statistical_outlier_removal.h>
#include "QRendView.h"

QLiqundian::QLiqundian(QWidget *parent)
	: QSubDialogBase(parent)
{
	ui.setupUi(this);

	ui.lineEdit_2->setValidator(new QIntValidator(this));
	ui.lineEdit_3->setValidator(new QDoubleValidator(this));

	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  
	ui.lineEdit_2->setText(QString::fromLocal8Bit(pt.get<std::string>("��Ⱥ��.k").c_str()));
	ui.lineEdit_3->setText(QString::fromLocal8Bit(pt.get<std::string>("��Ⱥ��.ƽ������").c_str()));
}

QLiqundian::~QLiqundian()
{
}

void QLiqundian::OnApply()
{
	QRendView* ins = QRendView::MainRendView();
	PointCloudT::Ptr cloud = PCManage::ins().cloud_;
	PointT minPt, maxPt;

	std::string k = ui.lineEdit_2->text().toLocal8Bit().data();
	std::string avgDistance = ui.lineEdit_3->text().toLocal8Bit().data();

	// �����˲�������ÿ����������ٽ���ĸ�������Ϊ50 ��������׼��ı�������Ϊ1  ����ζ�����һ
	//����ľ��볬����ƽ������һ����׼�����ϣ���õ㱻���Ϊ��Ⱥ�㣬�������Ƴ����洢����
	pcl::StatisticalOutlierRemoval<pcl::PointXYZRGB> sor;   //�����˲�������
	sor.setInputCloud(cloud);                           //���ô��˲��ĵ���
	sor.setMeanK(atoi(k.c_str()));                               //�����ڽ���ͳ��ʱ���ǲ�ѯ���ٽ�����
	sor.setStddevMulThresh(atof(avgDistance.c_str()));                      //�����ж��Ƿ�Ϊ��Ⱥ��ķ�ֵ
	sor.filter(*cloud);                    //�洢


	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);
	pt.put<std::string>("��Ⱥ��.k", k);  // д�ֶ�  
	pt.put<std::string>("��Ⱥ��.ƽ������", avgDistance);  // д�ֶ�  
    boost::property_tree::ini_parser::write_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  

	ins->UpdateView();
	this->accept();
}

void QLiqundian::Fun(PointCloudT::Ptr srcCloud /*= nullptr*/)
{
	PointCloudT::Ptr cloud = PCManage::ins().cloud_;
	if (nullptr != srcCloud)
	{
		cloud = srcCloud;
	}
	
	PointT minPt, maxPt;

	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  
	std::string k = pt.get<std::string>("��Ⱥ��.k");
	std::string avgDistance = pt.get<std::string>("��Ⱥ��.ƽ������");

	// �����˲�������ÿ����������ٽ���ĸ�������Ϊ50 ��������׼��ı�������Ϊ1  ����ζ�����һ
	//����ľ��볬����ƽ������һ����׼�����ϣ���õ㱻���Ϊ��Ⱥ�㣬�������Ƴ����洢����
	pcl::StatisticalOutlierRemoval<pcl::PointXYZRGB> sor;   //�����˲�������
	sor.setInputCloud(cloud);                           //���ô��˲��ĵ���
	sor.setMeanK(atoi(k.c_str()));                               //�����ڽ���ͳ��ʱ���ǲ�ѯ���ٽ�����
	sor.setStddevMulThresh(atof(avgDistance.c_str()));                      //�����ж��Ƿ�Ϊ��Ⱥ��ķ�ֵ
	sor.filter(*cloud);                    //�洢

}
