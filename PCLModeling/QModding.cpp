#include "QModding.h"
#include <pcl/features/normal_3d.h>
#include <pcl/search/kdtree.h>
#include <pcl/surface/gp3.h>
#include "QRendView.h"
#include "PCManage.h"

QModding::QModding(QWidget *parent)
	: QSubDialogBase(parent)
{
	ui.setupUi(this);
	ui.lineEdit->setValidator(new QIntValidator(this));
	ui.lineEdit_2->setValidator(new QDoubleValidator(this));
	ui.lineEdit_3->setValidator(new QDoubleValidator(this));
	ui.lineEdit_4->setValidator(new QIntValidator(this));
	ui.lineEdit_5->setValidator(new QDoubleValidator(this));
	ui.lineEdit_6->setValidator(new QDoubleValidator(this));
	ui.lineEdit_7->setValidator(new QDoubleValidator(this));

	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  
	ui.lineEdit  ->setText(QString::fromLocal8Bit(pt.get<std::string>("����ģ��.kd��kֵ").c_str()));
	ui.lineEdit_2->setText(QString::fromLocal8Bit(pt.get<std::string>("����ģ��.�����뾶").c_str()));
	ui.lineEdit_3->setText(QString::fromLocal8Bit(pt.get<std::string>("����ģ��.��Ȩ����").c_str()));
	ui.lineEdit_4->setText(QString::fromLocal8Bit(pt.get<std::string>("����ģ��.�ٽ�����ֵ").c_str()));
	ui.lineEdit_5->setText(QString::fromLocal8Bit(pt.get<std::string>("����ģ��.��Ƕ�").c_str()));
	ui.lineEdit_6->setText(QString::fromLocal8Bit(pt.get<std::string>("����ģ��.��������С�Ƕ�").c_str()));
	ui.lineEdit_7->setText(QString::fromLocal8Bit(pt.get<std::string>("����ģ��.���������Ƕ�").c_str()));
}

QModding::~QModding()
{
}

void QModding::OnApply()
{
	QRendView* ins = QRendView::MainRendView();
	PointCloudT::Ptr cloud = PCManage::ins().cloud_;


	//�趨�ᾧ����
	int KSearch =/* 0.008f*/atof(ui.lineEdit->text().toLocal8Bit().data());
	float SearchRadius = /*0.1f*/atof(ui.lineEdit_2->text().toLocal8Bit().data());
	float Mu = /*0.2f*/atof(ui.lineEdit_3->text().toLocal8Bit().data());
	int MaximumNearestNeighbors = /*0.4f*/atof(ui.lineEdit_4->text().toLocal8Bit().data());
	float MaximumSurfaceAngle = /*1.0f*/atof(ui.lineEdit_5->text().toLocal8Bit().data());
	float MinimumAngle = /*1.0f*/atof(ui.lineEdit_6->text().toLocal8Bit().data());
	float MaximumAngle = /*1.0f*/atof(ui.lineEdit_7->text().toLocal8Bit().data());

	// ���㷨��*
	pcl::NormalEstimation<PointT, pcl::Normal> n;
	pcl::PointCloud<pcl::Normal>::Ptr normals(new pcl::PointCloud<pcl::Normal>);
	pcl::search::KdTree<PointT>::Ptr tree(new pcl::search::KdTree<PointT>);
	tree->setInputCloud(cloud);
	n.setInputCloud(cloud);
	n.setSearchMethod(tree);
	n.setKSearch(/*20*/KSearch);	//20
	n.compute(*normals);
	//* normals should not contain the point normals + surface curvatures

	// Concatenate the XYZ and normal fields*
	pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud_with_normals(new pcl::PointCloud<pcl::PointXYZRGBNormal>);
	pcl::concatenateFields(*cloud, *normals, *cloud_with_normals);
	//* cloud_with_normals = cloud + normals

	// Create search tree*
	pcl::search::KdTree<pcl::PointXYZRGBNormal>::Ptr tree2(new pcl::search::KdTree<pcl::PointXYZRGBNormal>);
	tree2->setInputCloud(cloud_with_normals);

	// Initialize objects
	pcl::GreedyProjectionTriangulation<pcl::PointXYZRGBNormal> gp3;
	pcl::PolygonMesh triangles;

	// Set the maximum distance between connected points (maximum edge length)
	// �����뾶���趨����������������û�ָ���������������ؽ��������εĴ�С0.025
	gp3.setSearchRadius(/*0.025*/SearchRadius);

	// Set typical values for the parameters
	// mu�Ǹ���Ȩ���ӣ�����ÿ���ο��㣬��ӳ����ѡ��İ뾶��mu����ο��������ľ���˻��������������ͺܺý���˵����ܶȲ����ȵ����⣬muһ��ȡֵΪ2.5-3��
	gp3.setMu(/*2.5*/Mu);
	// �ٽ�����ֵ�趨��һ��Ϊ80-100��
	gp3.setMaximumNearestNeighbors(/*100*/MaximumNearestNeighbors);

	// ����ķ������ǶȲ���ڴ�ֵ�������㽫�������ӳ������Σ������ǡǡ�͵��ƾֲ�ƽ����Լ����Ӧ�����һ�����Ǽ������ô�����������Χ�ĵ���������Σ�
	// ��ʵ���Ҳ��Ч���˵���һЩ��Ⱥ�㡣���ֵһ��Ϊ45�ȡ�
	gp3.setMaximumSurfaceAngle(/*M_PI / 4*/MaximumSurfaceAngle); // 45 degrees
	// �����������С�Ƕȵ���ֵ��
	gp3.setMinimumAngle(/*M_PI / 18*/MinimumAngle); // 10 degrees
	gp3.setMaximumAngle(/*2 * M_PI / 3*/MaximumAngle); // 120 degrees
	//����ķ������Ƿ������仯�ġ����һ��������false����������ĵ�����ȫ�ֹ⻬�ģ�����˵һ���򣩡�
	gp3.setNormalConsistency(false);

	// Get result
	gp3.setInputCloud(cloud_with_normals);
	gp3.setSearchMethod(tree2);
	gp3.reconstruct(triangles);

	ins->viewer_->addPolygonMesh(triangles, "triangles");  //������Ҫ��ʾ���������
	//��������ģ����ʾģʽ
	//viewer_->setRepresentationToSurfaceForAllActors(); //����ģ������Ƭ��ʽ��ʾ  
	//viewer_->setRepresentationToPointsForAllActors(); //����ģ���Ե���ʽ��ʾ  
	ins->viewer_->setRepresentationToWireframeForAllActors();  //����ģ�����߿�ͼģʽ��ʾ
	ins->viewer_->resetCamera();



	boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);
	pt.put<int>("����ģ��.kd��kֵ", KSearch);
	pt.put<float>("����ģ��.�����뾶", SearchRadius);
	pt.put<float>("����ģ��.��Ȩ����", Mu);
	pt.put<int>("����ģ��.�ٽ�����ֵ", MaximumNearestNeighbors);
	pt.put<float>("����ģ��.��Ƕ�", MaximumSurfaceAngle);
	pt.put<float>("����ģ��.��������С�Ƕ�", MinimumAngle);
	pt.put<float>("����ģ��.���������Ƕ�", MaximumAngle);
    boost::property_tree::ini_parser::write_ini((QApplication::applicationDirPath() + QStringLiteral("/user.ini")).toLocal8Bit().data(), pt);  // �򿪶��ļ�  

// 	pcl::io::saveVTKFile("mesh.vtk", triangles);
// 	pcl::io::savePLYFile("mesh.ply", triangles);
	//ins->UpdateView();
	this->accept();
}
