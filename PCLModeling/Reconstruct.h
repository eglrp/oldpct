#pragma once
/**
*  ��ά�ؽ���                                             
**/
#include "PCManage.h"

#ifndef IN
#define IN
#endif

class Reconstruct
{
public:
    static void BuildSceneMesh(IN PointCloudT::Ptr cloud);

    // pcl �ɰ��ؽ�
    static void PCLPoissonReconstruct(PointCloudT::Ptr color_cloud);

    // pcl ��ά�������㷨�ؽ�
    static void PclCubeReconstruct(PointCloudT::Ptr color_cloud);

    // pcl ̰��ͶӰ���ǻ��㷨
    static void PclGp3Reconstruct(PointCloudT::Ptr color_cloud);

    // cgal �������㷨
    static void CGALReconstruct(PointCloudT::Ptr cloud);
};


