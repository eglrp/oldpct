#include "QResampling.h"
#include <pcl/surface/mls.h>        //��С���˷�ƽ�������ඨ��ͷ�ļ�
//������͵�ͷ�ļ�
#include <pcl/point_types.h>
//�����ļ�IO��pcd�ļ���ply�ļ���
#include <pcl/io/pcd_io.h>
#include <pcl/io/ply_io.h>
//kd��
#include <pcl/kdtree/kdtree_flann.h>
//������ȡ
#include <pcl/features/normal_3d_omp.h>
#include <pcl/features/normal_3d.h>
#include <pcl/surface/marching_cubes_hoppe.h>
#include <pcl/surface/marching_cubes_rbf.h>
//�ع�
#include <pcl/surface/gp3.h>
#include <pcl/surface/poisson.h>
//���ӻ�
#include <pcl/visualization/pcl_visualizer.h>
//���߳�
#include <boost/thread/thread.hpp>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <QDoubleValidator>
#include <QRendView.h>
#include <PCManage.h>

QResampling::QResampling(QWidget *parent)
    : QSubDialogBase(parent)
{
    ui.setupUi(this);
    ui.lineEdit->setValidator(new QDoubleValidator(this));
}

QResampling::~QResampling()
{
}



void QResampling::PclMlsReconstruct(double k/* = 5*/, PointCloudT::Ptr color_cloud /*= nullptr*/)
{
    QRendView* ins = QRendView::MainRendView();
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>());

    if (!color_cloud)
    {
        color_cloud = PCManage::ins().cloud_;
    }
    
    cloud->resize(color_cloud->size());
    for (int i = 0; i < cloud->size(); ++i)
    {
        cloud->at(i).x = color_cloud->at(i).x;
        cloud->at(i).y = color_cloud->at(i).y;
        cloud->at(i).z = color_cloud->at(i).z;
    }


    // ���� KD-Tree
    pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);

    // Output has the PointNormal type in order to store the normals calculated by MLS
    pcl::PointCloud<pcl::PointNormal> mls_points;

    // ������С����ʵ�ֵĶ���mls
    pcl::MovingLeastSquares<pcl::PointXYZ, pcl::PointNormal> mls;

    mls.setComputeNormals(true);  //��������С���˼�������Ҫ���з��߹���

    // Set parameters
    mls.setInputCloud(cloud);
    mls.setPolynomialFit(true);
    mls.setSearchMethod(tree);
    mls.setSearchRadius(k);
    mls.setUpsamplingRadius(2);

    // Reconstruct
    mls.process(mls_points);

    color_cloud->clear();
    color_cloud->resize(mls_points.size());
    for (int i = 0; i < color_cloud->size(); ++i)
    {
        color_cloud->at(i).x = mls_points.at(i).x;
        color_cloud->at(i).y = mls_points.at(i).y;
        color_cloud->at(i).z = mls_points.at(i).z;
    }

    // Save output
    //pcl::io::savePCDFile("bun0-mls.pcd", mls_points);
    ins->UpdateView();
}

void QResampling::on_click()
{
    PclMlsReconstruct(ui.lineEdit->text().toDouble());
    accept();
}
