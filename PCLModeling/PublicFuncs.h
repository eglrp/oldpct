#pragma once
#include "MathGeoLibFwd.h"
#include "Math/float3.h"
#include <boost/shared_ptr.hpp>
#include "PCManage.h"

#ifndef IN
#define IN
#endif

// �������ĵ�
PointT GetCenter(const PointCloudT cloud);

// vecתpointt
PointT Vec2PointT(const vec& pt);

//pointtתvec
vec PointT2Vec(const PointT& pt);

// ת�����ֲ�����ϵ����Ϊ��˹����ϵ����ֵ̫���޷�����obb��Χ����������������
void Transitionlocal(PointCloudT &cloud, IN vec const *centerptr = nullptr);

// xyz�ļ�תΪlas�ļ�
void Xyz2Las(const std::string & infile, const std::string &outfile);