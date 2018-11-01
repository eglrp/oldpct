#include "QPowerLineClassification.h"
#include <pcl/common/common.h>
#include <pcl/io/pcd_io.h>
#include "QRendView.h"
#include "PCManage.h"
#include <QColor>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/features/normal_3d.h>
#include <pcl/surface/gp3.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/filters/project_inliers.h>
#include <pcl/ModelCoefficients.h>
#include <QMessageBox>
#include <pcl/surface/mls.h>
#include <pcl/search/kdtree.h>
#include <QQuyuzengzhangNormal.h>
#include <QQuyuzengzhangColor.h>
#include <QOushifenge.h>
#include <pcl/filters/voxel_grid.h>
#include <QSimplify.h>
#include <QGround.h>
#include <QLiqundian.h>
#include <vtkPointSet.h>
#include <vtkOBBTree.h>
#include <Eigen\src\Core\Matrix.h>
#include "dbscan.h"
#include <algorithm>
#include <boost/format.hpp>
#include <omp.h>
#include "MathGeoLib/Geometry/OBB.h"
#include "dvprint.h"
#include "LeastSquare.h"
#include "ProjectionXyPlane.h"
#include <vtkPolyLine.h>
#include <pcl/range_image/range_image.h>
#include "LineDetection3D/LineDetection3D.h"
#include "vtkActor.h"  
#include "vtkCamera.h"  
#include "vtkCellArray.h"  
#include "vtkPoints.h"  
#include "vtkPolyData.h"  
#include "vtkPolyDataMapper.h"  
#include "vtkRenderWindow.h"  
#include "vtkRenderWindowInteractor.h"  
#include "vtkRenderer.h"   
#include "vtkProperty.h"  
#include "vtkTubeFilter.h"  
#include "vtkParametricSpline.h"  
#include "vtkParametricFunctionSource.h" 
#include "vtkPolyLine.h"
#include <vtkTupleInterpolator.h>
#include <vtkDoubleArray.h>


int GetHistogram(PointCloudT &cloud,double *(&hist), PointT minPt, PointT maxPt, double gridHeight)
{
	int cloudSize = cloud.size();
	if (gridHeight == 0)
		return 0;
	int Histogramdim = (maxPt.z - minPt.z) / gridHeight + 1;
	hist = new double[Histogramdim];
	memset(hist, 0, sizeof(double)*Histogramdim);
	for (int k = 0; k < cloudSize; ++k)
	{
		hist[(int)((cloud[k].z - minPt.z) / gridHeight)]++;
	}

	for (int k = 0; k < Histogramdim; k++)
	{
		hist[k] = hist[k] / cloudSize;
	}

	return Histogramdim;
}

int GetHistogram(std::vector<std::vector<int>> &densitys, std::vector<double> &hist, int minCount, int maxCount/*, double gridHeight*/)
{
	std::map<int, double> mhist;
	

	int rows = densitys.size();
	int cols = densitys[0].size();
	int gridCount = 0;

	for (int k = 0; k <rows; ++k)
	{
		for (int j = 0; j < cols; ++j)
		{
			if(!densitys[k][j])
				continue;
			++gridCount;
			mhist[(densitys[k][j] - minCount)]++;
		}
	}

	for (auto it = mhist.begin(); it != mhist.end(); ++it)
	{
		it->second = it->second / gridCount;
		hist.push_back(it->second);
	}

	return hist.size();
}


int Otsu(std::vector<double> &hist)
{
    if (!hist.size())
        return 0;


    int Histogramdim = hist.size();

    double *omega = new double[Histogramdim]{ 0 };
    double *mu = new double[Histogramdim]{ 0 };

    omega[0] = hist[0];
    mu[0] = 0;
    for (int i = 1; i < Histogramdim; i++)
    {
        omega[i] = omega[i - 1] + hist[i]; //�ۻ��ֲ�����
        mu[i] = mu[i - 1] + i * hist[i];
    }
    double mean = mu[Histogramdim - 1];// �Ҷ�ƽ��ֵ
    double max = 0;
    int k_max = 0;
    for (int k = 1; k < Histogramdim - 1; k++)
    {
        double PA = omega[k]; // A����ռ�ı���
        double PB = 1 - omega[k]; //B����ռ�ı���
        double value = 0;
        if (fabs(PA) > 0.001 && fabs(PB) > 0.001)
        {
            double MA = mu[k] / PA; //A ��ĻҶȾ�ֵ
            double MB = (mean - mu[k]) / PB;//B��ҶȾ�ֵ
            value = PA * (MA - mean) * (MA - mean) + PB * (MB - mean) * (MB - mean);//��䷽��

            if (value > max)
            {
                max = value;
                k_max = k;
            }
        }
    }
    return k_max;
}

int Otsu(std::vector<std::vector<int>> &densitys)
{
	if (!densitys.size() || !densitys[0].size())
		return 0;

	std::vector<double> hist;
	int minCount = std::numeric_limits<int>::max();
	int maxCount = -std::numeric_limits<int>::max();


	for (int i = 0; i < densitys.size(); ++i)
	{
		for (int j = 0; j < densitys[i].size(); ++j)
		{
			int curCount = densitys[i][j];
			if (curCount == 0)
				continue;
			
			if (curCount > maxCount)
				maxCount = curCount;
			if (curCount < minCount)
				minCount = curCount;
		}
	}

	int Histogramdim = GetHistogram(densitys, hist, minCount, maxCount);
	dd("mincount = %d, maxcound = %d, GetHistogram = %d", minCount, maxCount, Histogramdim);

	double *omega = new double[Histogramdim]{ 0 };
	double *mu = new double[Histogramdim]{ 0 };

	omega[0] = hist[0];
	mu[0] = 0;
	for (int i = 1; i < Histogramdim; i++)
	{
		omega[i] = omega[i - 1] + hist[i]; //�ۻ��ֲ�����
		mu[i] = mu[i - 1] + i * hist[i];
	}
	double mean = mu[Histogramdim - 1];// �Ҷ�ƽ��ֵ
	double max = 0;
	int k_max = 0;
	for (int k = 1; k < Histogramdim - 1; k++)
	{
		double PA = omega[k]; // A����ռ�ı���
		double PB = 1 - omega[k]; //B����ռ�ı���
		double value = 0;
		if (fabs(PA) > 0.001 && fabs(PB) > 0.001)
		{
			double MA = mu[k] / PA; //A ��ĻҶȾ�ֵ
			double MB = (mean - mu[k]) / PB;//B��ҶȾ�ֵ
			value = PA * (MA - mean) * (MA - mean) + PB * (MB - mean) * (MB - mean);//��䷽��

			if (value > max)
			{
				max = value;
				k_max = k;
			}
		}
	}
	return k_max;
}

// #define Histogramdim 256
int Otsu(PointCloudT &cloud, PointT minPt, PointT maxPt, double gridHeight)
{
	if (gridHeight == 0)
		return 0;

	double *hist = nullptr;
	int Histogramdim = GetHistogram(cloud, hist, minPt, maxPt, gridHeight);
// 	std::vector<double> s;
// 	for (int i = 0; i < Histogramdim; ++i)
// 	{
// 		s.push_back(hist[i]);
// 	}
	double *omega = new double[Histogramdim]{ 0 };
	double *mu = new double[Histogramdim]{ 0 };

	omega[0] = hist[0];
	mu[0] = 0;
	for (int i = 1; i < Histogramdim; i++)
	{
		omega[i] = omega[i - 1] + hist[i]; //�ۻ��ֲ�����
		mu[i] = mu[i - 1] + i * hist[i];
	}
	double mean = mu[Histogramdim - 1];// �Ҷ�ƽ��ֵ
	double max = 0;
	int k_max = 0;
	for (int k = 1; k < Histogramdim - 1; k++)
	{
		double PA = omega[k]; // A����ռ�ı���
		double PB = 1 - omega[k]; //B����ռ�ı���
		double value = 0;
		if (fabs(PA) > 0.001 && fabs(PB) > 0.001)
		{
			double MA = mu[k] / PA; //A ��ĻҶȾ�ֵ
			double MB = (mean - mu[k]) / PB;//B��ҶȾ�ֵ
			value = PA * (MA - mean) * (MA - mean) + PB * (MB - mean) * (MB - mean);//��䷽��

			if (value > max)
			{
				max = value;
				k_max = k;
			}
		}
		//qDebug() <<k << " " << hist[k] << " " << value;
	}
	delete []hist;
	return k_max;
}

void setColors(PointCloudT &cloud, std::vector<int> &indices, uchar r = 255, uchar g = 255, uchar b = 0)
{
	for (int i = 0; i < indices.size(); ++i)
	{
		auto & pt = cloud.at(indices[i]);
		pt.r = r;
		pt.g = g;
		pt.b = b;
	}
}

void setColors(PointCloudT &cloud, uchar r = 255, uchar g = 255, uchar b = 0)
{
	for (int i = 0; i < cloud.size(); ++i)
	{
		auto & pt = cloud.at(i);
		pt.r = r;
		pt.g = g;
		pt.b = b;
	}
}


QPowerLineClassification::QPowerLineClassification(QWidget *parent)
: QSubDialogBase(parent)
{
	
 	ui.setupUi(this);
	ui.lineEdit->setValidator(new QDoubleValidator(this));
	ui.lineEdit_2->setValidator(new QDoubleValidator(this));

	ui.lineEdit_9->setValidator(new QDoubleValidator(this));
	ui.lineEdit_11->setValidator(new QDoubleValidator(this));
	ui.lineEdit_10->setValidator(new QIntValidator(this));


	boost::property_tree::ptree pt;
	boost::property_tree::ini_parser::read_ini("user.ini", pt);  // �򿪶��ļ�  
	ui.groupBox->setChecked(pt.get<bool>("������ʶ��.��ظ߶ȹ���"));
	ui.lineEdit_2->setText(QString::fromLocal8Bit(pt.get<std::string>("������ʶ��.��ߵ������ľ���<x��ɾ��").c_str()));

	ui.groupBox_2->setChecked(pt.get<bool>("������ʶ��.��Χ�й���"));
	ui.lineEdit_9->setText(QString::fromLocal8Bit(pt.get<std::string>("������ʶ��.����߶�С��x��ɾ��").c_str()));

	ui.groupBox_3->setChecked(pt.get<bool>("������ʶ��.�ܶȹ���"));
	ui.lineEdit_11->setText(QString::fromLocal8Bit(pt.get<std::string>("������ʶ��.�ܶȿռ�ṹҶ�ӽڵ�ֱ��").c_str()));
	ui.lineEdit_10->setText(QString::fromLocal8Bit(pt.get<std::string>("������ʶ��.�����ܶ�ʱ�����������").c_str()));
	ui.lineEdit->setText(QString::fromLocal8Bit(pt.get<std::string>("������ʶ��.�ܶ���ֵ").c_str()));

    ui.lineEdit_6->setText(QString::fromLocal8Bit(pt.get<std::string>("������ʶ��.�ϲ������߸߲�").c_str()));
    ui.lineEdit_5->setText(QString::fromLocal8Bit(pt.get<std::string>("������ʶ��.�ϲ������߾���").c_str()));

    ui.lineEdit_7->setText(QString::fromLocal8Bit(pt.get<std::string>("������ʶ��.����������").c_str()));
	OnGroupBoxCheck(false);

	InitData();
    InitCloudInfo();
    UpdateShowClass("����");
    UpdateView();
}

void QPowerLineClassification::InitData()
{
	cloud_.reset();
	cloud_ = boost::make_shared<PointCloudT>();
    ground_.reset();
    ground_ = boost::make_shared<PointCloudT>();

    *cloud_ = *PCManage::ins().cloud_;
}

void readDataFromPCL(PointCloudT::Ptr incloud, PointCloud<double> &cloud)
{
    cloud.pts.reserve(incloud->size());

    for (int i = 0; i < incloud->size(); ++i)
    {
        PointT &pt = incloud->at(i);
        cloud.pts.push_back(PointCloud<double>::PtData(pt.x, pt.y, pt.z));
     }
}

void QPowerLineClassification::ExtractGround()
{
    QSimplify::Fun(cloud_); 
    QGround::Fun(cloud_, *ground_);
    RecoverGround(ground_, cloud_);
    RoughExtractGroundObject();
 
    UpdateShowClass("����");
    UpdateView();
}

void QPowerLineClassification::GroundObjectFilter()
{
    bool groudLowerFilter = ui.groupBox->isChecked();
	bool aabbBoxFilter = ui.groupBox_2->isChecked();
	bool densityFilter = ui.groupBox_3->isChecked();
    double density = atof(ui.lineEdit->text().toLocal8Bit().data());
    double leafSize = atof(ui.lineEdit_11->text().toLocal8Bit().data());
    int neighborNum = atoi(ui.lineEdit_10->text().toLocal8Bit().data());
    double groundLower = atof(ui.lineEdit_2->text().toLocal8Bit().data());
    double aabbLimit = atof(ui.lineEdit_9->text().toLocal8Bit().data());

    std::vector <pcl::PointIndices> &jlClusters = PCManage::ins().jlClusters_;
    PointCloudT::Ptr cloud = cloud_;

    pcl::KdTreeFLANN<PointT> kdtree;
    kdtree.setInputCloud(ground_);

    std::vector<PointCloudT::Ptr> filterVec(jlClusters.size(), nullptr);

#pragma omp parallel for
    for (int i = 0; i < jlClusters.size(); ++i)
    {
        PointCloudT::Ptr curCloud(new PointCloudT);
        pcl::PointIndices::Ptr curindices(new pcl::PointIndices());
        *curindices = jlClusters[i];
        pcl::ExtractIndices<PointT> extract;
        extract.setInputCloud(cloud);
        extract.setIndices(curindices);
        extract.filter(*curCloud);

        if (groudLowerFilter && ground_->size()
            && IsLowerClass(*curCloud, kdtree, groundLower/*, groundLnterval, aabbHeight*/)/*IsLowerClass(*curCloud, kdtree, 15,10,30)*/)
        {
            filterVec[i] = curCloud;
        }
         else if (aabbBoxFilter && IsAABBUnusual(*curCloud, aabbLimit))
         {
             filterVec[i] = curCloud;
         }
         else if (densityFilter && IsHighDensity(*curCloud, density, leafSize, neighborNum))
         {
             filterVec[i] = curCloud;
         }
    }

    std::vector <pcl::PointIndices>::iterator it = jlClusters.begin();
    std::vector<PointCloudT::Ptr>::iterator fit = filterVec.begin();
    for (; it != jlClusters.end();)
    {
        if (nullptr != *fit)
        {
            it = jlClusters.erase(it);
            ++fit;
        }
        else
        {
            fit = filterVec.erase(fit);
            ++it;
        }
    }

    groundObjects_.insert(groundObjects_.end(), filterVec.begin(), filterVec.end());


    dd("�������лָ�ǰ��%d", groundObjects_.size());
    RecoverGroundObject();
    dd("�������лָ���%d", groundObjects_.size());
	UpdateJLClusters2Cloud();
}

QPowerLineClassification::~QPowerLineClassification()
{
	SaveSetting();
	//DestroyData();
}

// �����ƴ����ά����
void QPowerLineClassification::PutPointCloud2Arr(PointCloudT::Ptr cloud, std::vector<std::vector<pcl::PointIndices::Ptr>> &pointsArr, int row, int col, PointT min, PointT max, double gridSize)
{
	// �������
	for (int i = 0; i < cloud->size(); ++i)
	{
		PointT &pt = cloud->at(i);
        pcl::PointIndices::Ptr &indices = pointsArr[(int)((pt.x - min.x) / gridSize)][(int)((pt.y - min.y) / gridSize)];
        if (!indices)
            indices = boost::make_shared<pcl::PointIndices>();
        indices->indices.push_back(i);
	}
}

// �����ƴ����ά����
void QPowerLineClassification::PutPointCloud2Arr(
    PointCloudT::Ptr cloud, 
    std::vector<std::vector<pcl::PointIndices::Ptr>> &pointsArr,
    PointCloudT::Ptr ground,
    std::vector<std::vector<pcl::PointIndices::Ptr>> &groundArr,
    int row, 
    int col, 
    PointT min,
    PointT max, 
    double gridSize )
{
#pragma omp parallel sections
    {
#pragma omp section
        {
            for (int i = 0; i < cloud->size(); ++i)
            {
                PointT &pt = cloud->at(i);
                pcl::PointIndices::Ptr &indices = pointsArr[(int)((pt.x - min.x) / gridSize)][(int)((pt.y - min.y) / gridSize)];
                if (!indices)
                    indices = boost::make_shared<pcl::PointIndices>();
                indices->indices.push_back(i);
            }
        }
#pragma omp section
        {
            for (int i = 0; i < ground->size(); ++i)
            {
                PointT &pt = ground->at(i);
                if (pt.x < min.x || pt.x > max.x || pt.y < min.y || pt.y > max.y)
                    continue;
                pcl::PointIndices::Ptr &indices = groundArr[(int)((pt.x - min.x) / gridSize)][(int)((pt.y - min.y) / gridSize)];
                if (!indices)
                    indices = boost::make_shared<pcl::PointIndices>();
                indices->indices.push_back(i);
            }
        }
    }
}

void QPowerLineClassification::NoRepeat(std::vector<uint>& indices)
{
    std::set<int> norepeat;

    for (int a = 0; a < indices.size(); ++a)
    {
        norepeat.insert(indices[a]);
    }
    indices.clear();
    for (auto it = norepeat.begin(); it != norepeat.end(); ++it)
    {
        indices.push_back(*it);
    }
}

void QPowerLineClassification::NoRepeat(pcl::PointIndices::Ptr vecrangePoint)
{
	std::set<int> norepeat;

	for (int a = 0; a < vecrangePoint->indices.size(); ++a)
	{
		norepeat.insert(vecrangePoint->indices[a]);
	}
	vecrangePoint->indices.clear();
	for (std::set<int>::iterator it = norepeat.begin(); it != norepeat.end(); ++it)
	{
		vecrangePoint->indices.push_back(*it);
	}
}

void QPowerLineClassification::SerachVecRangePoints(PointT pt, double dis, double minz, double maxz, pcl::PointIndices::Ptr vecrangePoint)
{
    for (double k = minz; k < maxz; k++)
    {
        pt.z = k;
        pcl::PointIndices::Ptr tempIndices(new pcl::PointIndices);
        pcl::KdTreeFLANN<PointT> kdtree;
        kdtree.setInputCloud(cloud_);
        std::vector<int> indices;
        std::vector<float> sqr_distances;

        if (kdtree.radiusSearch(pt, dis, indices, sqr_distances))
        {
            //vecrangePoint->indices = indices;
            vecrangePoint->indices.insert(vecrangePoint->indices.end(), indices.begin(), indices.end());
        }
    }
}

void QPowerLineClassification::SerachVecRangePoints(PointT pt, double dis, pcl::PointIndices::Ptr vecPoints, pcl::PointIndices::Ptr vecrangePoint)
{
	Eigen::Vector4f min_towerptv4, max_towerptv4;
	pcl::getMinMax3D(*cloud_, vecPoints->indices, min_towerptv4, max_towerptv4);
	double maxz = max_towerptv4.z();
	double minz = min_towerptv4.z();

	for (int k = minz; k < maxz; k++)
	{
		pt.z = k;
		pcl::PointIndices::Ptr tempIndices(new pcl::PointIndices);
		pcl::KdTreeFLANN<PointT> kdtree;
		kdtree.setInputCloud(cloud_);
		std::vector<int> indices;
		std::vector<float> sqr_distances;
	
		if (kdtree.radiusSearch(pt, dis, indices, sqr_distances))
		{
			vecrangePoint->indices.insert(vecrangePoint->indices.end(), indices.begin(), indices.end());
            //vecrangePoint->indices = indices;
		}
	}
}

vec QPowerLineClassification::GetObbBox(PointCloudT &cloud)
{
    int ptct = cloud.size();
    boost::shared_ptr<vec> points(new vec[ptct], std::default_delete<vec[]>());
    for (int j = 0; j < ptct; ++j)
    {
        PointT &pt = cloud[j];
        points.get()[j] = vec(pt.x, pt.y, pt.z) - info_.center;
    }
    OBB obb;
    obb = OBB::BruteEnclosingOBB(points.get(), ptct);
    return obb.HalfDiagonal();
 	//pcl::MomentOfInertiaEstimation <PointT> feature_extractor;
// 	feature_extractor.setInputCloud(cloud.makeShared());
// 	feature_extractor.compute();
// 	feature_extractor.getOBB(min_point_OBB, max_point_OBB, position_OBB, rotational_matrix_OBB);
}

void QPowerLineClassification::DistanceSerach(double cenX, double cenY, double cenZ, double dis, std::vector<int>& indices)
{
    pcl::KdTreeFLANN<PointT> kdtree;
    kdtree.setInputCloud(cloud_);

    std::vector<float> indicesDistance;           //�洢���ڵ��Ӧƽ������
    PointT pt;
    pt.x = cenX;
    pt.y = cenY;
    pt.z = cenZ;
    kdtree.radiusSearch(pt, dis, indices, indicesDistance);
}

std::vector<PointT> vec8toPoint8(vec *v8)
{
    std::vector<PointT> res(8);
    for (int i = 0; i < 8; ++i)
    {
        PointT pt;
        pt.x = v8[i].x;
        pt.y = v8[i].y;
        pt.z = v8[i].z;
        res[i] = pt;
    }
    return res;
}

float QPowerLineClassification::GetClustersHeight(const std::vector<uint> &clusters, const PointCloudT &cloud_)
{
    float minZ = std::numeric_limits<float>::max();
    float maxZ = -std::numeric_limits<float>::max();
    for (int i = 0; i < clusters.size(); ++i)
    {
        const float &curZ = cloud_[clusters[i]].z;
        if (curZ > maxZ)
        {
            maxZ = curZ;
        }
        if (curZ < minZ)
        {
            minZ = curZ;
        }
    }
    return  maxZ - minZ;
}

void QPowerLineClassification::ExtractTower()
{
    // ������ܶ�����
    struct vec2f {
        float data[2];
        vec2f(float x, float y){ data[0] = x; data[1] = y; };
        float operator[](int idx) const { return data[idx]; }
    };

    auto dbscan = DBSCAN<vec2f, float>();

    auto data = std::vector<vec2f>();
    for (int i = 0; i < cloud_->size(); ++i)
    {
        data.push_back(vec2f(cloud_->at(i).x, cloud_->at(i).y));
    }

    //���������ݣ� ά�ȣ���ά���� ���ǰ뾶�� ������С
    dbscan.Run(&data, 2, 1.0f, 30);
    auto noise = dbscan.Noise;
    auto clusters = dbscan.Clusters;

    dd("���ܶ���������%d", clusters.size());
    for (auto it = clusters.begin(); it != clusters.end(); )
    {
        if (!IsTower(cloud_, *it))
            it = clusters.erase(it);
        else
            ++it;
    }

    for (int i = 0; i < clusters.size() - 1; ++i)
    {
        for (int j = i + 1; j < clusters.size(); ++j)
        {
            if (!clusters[i].size())
                break;
            if (!clusters[j].size())
                continue;
            PointT& pt1 = cloud_->at(clusters[i].at(0));
            PointT& pt2 = cloud_->at(clusters[j].at(0));

            double dis = GetDistance2d(pt1.x, pt1.y, pt2.x, pt2.y);
            if (dis < 20)
            {
                dd("�ϲ����������ľ���%f<20", dis);
                clusters[j].insert(clusters[j].end(), clusters[i].begin(), clusters[i].end());
                clusters[i].clear();
            }
        }
    }

    // ɾ���յģ�ȥ���ظ��ģ�˳��ѵ���10�׵ġ�������ɾ����
    for (auto it = clusters.begin(); it != clusters.end();)
    {
        if ((*it).size() == 0 /*|| GetClustersHeight(*it, *cloud_) < 10*/)
            it = clusters.erase(it);
        else
        {
            NoRepeat(*it);
            ++it;
        }
    }

    dd("��������%d", clusters.size());
    std::vector<pcl::PointIndices::Ptr> towersIndies(clusters.size());

#pragma omp parallel for
    for (int i = 0; i < clusters.size(); ++i)
    {
        int ptct = clusters[i].size();
        boost::shared_ptr<vec> points(new vec[ptct], std::default_delete<vec[]>());
        for (int j = 0; j < clusters[i].size(); ++j)
        {
            uint &curindex = clusters[i][j];
            points.get()[j] = vec(cloud_->at(curindex).x, cloud_->at(curindex).y, cloud_->at(curindex).z) - info_.center;
        }

        // ��������obb
        OBB obb;
        vec diagonal;

// 
//         try
//         {
            obb = OBB::BruteEnclosingOBB(points.get(), ptct);
            obb.Scale(obb.pos, 1.5);
            diagonal = obb.HalfDiagonal();
//         }
//         catch (...)
//         {
//             OBBWB obbwb = GenerateOBB(points.get(), ptct);
//             obb.axis[0] = obbwb.orientation.a1;
//             obb.axis[1] = obbwb.orientation.a2;
//             obb.axis[2] = obbwb.orientation.a3;
//             obb.r = obbwb.halfExtents;
//             obb.pos = obbwb.position;
//         }
        for (int j = 0; j < 3; ++j)
        {
            if (obb.r[j] < 5)
                obb.r[j] = 5;
        }
        obb.pos += info_.center;
        std::vector<int> radiuIndices;
        DistanceSerach(obb.pos.x, obb.pos.y, obb.pos.z, diagonal.Length(), radiuIndices);
        towersIndies[i] = boost::make_shared<pcl::PointIndices>();
        for (int j = 0; j < radiuIndices.size(); ++j)
        {
            PointT &pt = cloud_->at(radiuIndices[j]);
            if (obb.Contains(vec(pt.x, pt.y, pt.z)))
            {
                pt.r = 148;
                pt.g = 0;
                pt.b = 211;
                towersIndies[i]->indices.push_back(radiuIndices[j]);
            }
        }

        dd("�ܶȾ���ĵ���%d����Χ�����ĵ���%d��obb��չ����%d��obb���ĵ�����%f,%f,%f����һ�������%f,%f,%f", ptct, radiuIndices.size(), towersIndies[i]->indices.size(), obb.pos.x, obb.pos.y, obb.pos.z, cloud_->at(clusters[i][0]).x, cloud_->at(clusters[i][0]).y, cloud_->at(clusters[i][0]).z);
    }

    // ��cloud_��ɾ����������
    pcl::PointIndices::Ptr removeIndices(new pcl::PointIndices());
    towers_.resize(towersIndies.size());
    for (int i = 0; i < towersIndies.size(); ++i)
    {
        towers_[i] = boost::make_shared<PointCloudT>();
        pcl::ExtractIndices<PointT> extract;
        extract.setInputCloud(cloud_);
        extract.setIndices(towersIndies[i]);
        extract.filter(*towers_[i]);
        removeIndices->indices.insert(removeIndices->indices.end(), towersIndies[i]->indices.begin(), towersIndies[i]->indices.end());
    }

    pcl::ExtractIndices<PointT> extract;
    extract.setInputCloud(cloud_);
    extract.setIndices(removeIndices);
    extract.setNegative(true);
    extract.filter(*cloud_);

    UpdateShowClass("����");
    UpdateShowClass("����");
    UpdateView();
}

bool QPowerLineClassification::CheckNormal(PointCloudT &cloud)
{
	//���ص���ģ��
	QRendView* ins = QRendView::MainRendView();
// 	PointCloudT::Ptr cloud = boost::make_shared<PointCloudT>();
// 	pcl::copyPoint(PCManage::ins().cloud_, cloud);
	//* the data should be available in cloud

	// Normal estimation*
	//�������
	pcl::NormalEstimation<PointT, pcl::Normal> n;
	pcl::PointCloud<pcl::Normal>::Ptr normals(new pcl::PointCloud<pcl::Normal>);
	//����kdtree�����н��ڵ㼯����
	pcl::search::KdTree<PointT>::Ptr tree(new pcl::search::KdTree<PointT>);
	//Ϊkdtree��ӵ�������
	tree->setInputCloud(cloud.makeShared());

	n.setInputCloud(cloud.makeShared());
	n.setSearchMethod(tree);
	//���Ʒ������ʱ����Ҫ�����Ľ��ڵ��С
	n.setKSearch(20);
	//n.setRadiusSearch(2);
	//��ʼ���з������
	n.compute(*normals);
	//* normals should not contain the point normals + surface curvatures



	int norSize = normals->size();
	int zsize = 0;
	for (int i = 0; i < norSize; ++i)
	{
		pcl::Normal n = normals->at(i);
		if (abs(n.normal_z) > 0.9 && abs(n.normal_x) < 0.1 && abs(n.normal_y) < 0.1)
		{
			++zsize;
		}
	}
	if (zsize / (double)norSize < 0.6)
 		return false;


 
	return true;
 	// Finish
}

bool QPowerLineClassification::IsHighDensity(PointCloudT &cloudsrc, float density, float leafSize, float neighborNum)
{
	// ��ʼ��kdTree
	// ����Ҫ�����ĵ���
	PointCloudT cloud = cloudsrc;
	pcl::VoxelGrid<PointT> grid;
	grid.setLeafSize(leafSize, leafSize, leafSize);
	grid.setInputCloud(cloud.makeShared());
   	grid.filter(cloud);


	pcl::KdTreeFLANN<PointT> kdtree;
	kdtree.setInputCloud(cloud.makeShared());
	//kdtree.setSortedResults(true);
	double res = 0.0;
	int n_points = 0;
	int nres;
	std::vector<int> indices;
	std::vector<float> sqr_distances;


	for (size_t i = 0; i < cloud.size(); ++i)
	{
		nres = kdtree.nearestKSearch(cloud[i], neighborNum, indices, sqr_distances);

		for (int j = 1; j < nres; ++j)
		{
			res += sqrt(sqr_distances[j]);
			++n_points;
		}
	}
	if (n_points != 0)
	{
		res /= n_points;
	}
    dd("��ǰ�ܶȣ�%f", res);
	if (res < density)
	{
		dd("���ܶȹ���-��ǰ�ܶȣ�%f", res);
		return true;
	}

	return false;
}


void QPowerLineClassification::RemoveHighDensityClass()
{
	double density = atof(ui.lineEdit->text().toLocal8Bit().data());
	double leafSize = atof(ui.lineEdit_11->text().toLocal8Bit().data());
	int neighborNum = atoi(ui.lineEdit_10->text().toLocal8Bit().data());

	PointCloudT::Ptr cloud = cloud_;
	std::vector <pcl::PointIndices> &jlClusters = PCManage::ins().jlClusters_;
	
	std::vector <pcl::PointIndices>::iterator it;
	for (it = jlClusters.begin(); it != jlClusters.end();)
	{
		PointCloudT::Ptr curCloud(new PointCloudT);
		pcl::PointIndices::Ptr curindices(new pcl::PointIndices());
		*curindices = *it;
		pcl::ExtractIndices<PointT> extract;
		extract.setInputCloud(cloud);
		extract.setIndices(curindices);
		extract.filter(*curCloud);

		if (IsHighDensity(*curCloud, density, leafSize, neighborNum))
		{
			it = jlClusters.erase(it);
            groundObjects_.push_back(curCloud);
		}
		else
		{
			++it;
		}
	}
}

bool QPowerLineClassification::IsLowerClass(PointCloudT &cloud, pcl::KdTreeFLANN<PointT> &ground_kdtree, double groundLower, double groundLnterval, double aabbHeight)
{
	PointT min, max;
	pcl::getMinMax3D(cloud, min, max);

	std::vector<int> indices;
	std::vector<float> sqr_distances;

	// ��ߵ�����ظ߶�<15������Ϊ�ǵ��棬ɾ����
    if (ground_kdtree.radiusSearch(max, groundLower, indices, sqr_distances))
	{
        //dd("��ߵ������ļ��%f<10,��ɾ��", sqr_distances[0]);
		return true;
	}

 	// �߳�С��30,����͵㵽����ļ��<10��Ŀ����ɾ����ľ������30�Ƿ�ֹɾ��������
    if ((max.z - min.z < aabbHeight) 
        && ( ground_kdtree.radiusSearch(min, groundLnterval, indices, sqr_distances)))
 	{
        //dd("��͵㵽����ļ��%f<10,�Ҹ߳�%fС��30 ", sqr_distances[1], max.z - min.z);
 		return true;
  	}

	return false;
}

bool QPowerLineClassification::IsLowerClass(PointCloudT &cloud, pcl::KdTreeFLANN<PointT> &ground_kdtree, double groundLower)
{
    PointT min, max;
    pcl::getMinMax3D(cloud, min, max);

    std::vector<int> indices;
    std::vector<float> sqr_distances;

    // ��ߵ�����ظ߶�<15������Ϊ�ǵ��棬ɾ����
    if (ground_kdtree.radiusSearch(max, groundLower, indices, sqr_distances))
    {
        //dd("��ߵ������ļ��%f<%f����ɾ��", sqr_distances[0]);
        return true;
    }

    return false;
}

void QPowerLineClassification::RemoveLowerClass()
{
	double groundLower = atof(ui.lineEdit_2->text().toLocal8Bit().data());


	PointCloudT::Ptr cloud = cloud_;
	std::vector <pcl::PointIndices> &jlClusters = PCManage::ins().jlClusters_;

    if (!ground_->size())
	{
		return;
	}

	pcl::KdTreeFLANN<PointT> kdtree;
    kdtree.setInputCloud(ground_);

	std::vector <pcl::PointIndices>::iterator it;
	for (it = jlClusters.begin(); it != jlClusters.end();)
	{
		PointCloudT::Ptr curCloud(new PointCloudT);
		pcl::PointIndices::Ptr curindices(new pcl::PointIndices());
		*curindices = *it;
		pcl::ExtractIndices<PointT> extract;
		extract.setInputCloud(cloud);
		extract.setIndices(curindices);
		extract.filter(*curCloud);


		if (IsLowerClass(*curCloud, kdtree, groundLower/*, groundLnterval, aabbHeight*/))
		{
			it = jlClusters.erase(it);
            groundObjects_.push_back(curCloud);
		}
		else
		{
			++it;
		}
	}
}

void QPowerLineClassification::UpdateJLClusters2Cloud()
{
	pcl::PointIndices::Ptr indicess(new pcl::PointIndices());
	std::vector <pcl::PointIndices> &jlClusters = PCManage::ins().jlClusters_;

	if (jlClusters.size())
	{
        std::vector <pcl::PointIndices>::iterator it = jlClusters.begin();
        indicess->header = it->header;
        for (it = jlClusters.begin(); it != jlClusters.end(); ++it)
        {
            indicess->indices.insert(indicess->indices.end(), it->indices.begin(), it->indices.end());
        }

        pcl::ExtractIndices<PointT> extract;
        extract.setInputCloud(cloud_);
        extract.setIndices(indicess);
        extract.filter(*cloud_);
	}

    UpdateShowClass("����");
    UpdateView();
}

void QPowerLineClassification::SaveSetting()
{
	// ������ȡ
	bool groudLowerFilter = ui.groupBox->isChecked();
	bool aabbBoxFilter = ui.groupBox_2->isChecked();
	bool densityFilter = ui.groupBox_3->isChecked();
	double density = atof(ui.lineEdit->text().toLocal8Bit().data());
	double groundLower = atof(ui.lineEdit_2->text().toLocal8Bit().data());

	double aabbLimit = atof(ui.lineEdit_9->text().toLocal8Bit().data());
	double leafSize = atof(ui.lineEdit_11->text().toLocal8Bit().data());
	int neighborNum = atoi(ui.lineEdit_10->text().toLocal8Bit().data());

	boost::property_tree::ptree pt;
	boost::property_tree::ini_parser::read_ini("user.ini", pt);
    pt.put<bool>("������ʶ��.��ظ߶ȹ���", groudLowerFilter);
    pt.put<bool>("������ʶ��.��Χ�й���", aabbBoxFilter);
    pt.put<bool>("������ʶ��.�ܶȹ���", densityFilter);
    pt.put<double>("������ʶ��.�ܶ���ֵ", density);
    pt.put<double>("������ʶ��.��ߵ������ľ���<x��ɾ��", groundLower);
    pt.put<double>("������ʶ��.����߶�С��x��ɾ��", aabbLimit);
    pt.put<double>("������ʶ��.�ܶȿռ�ṹҶ�ӽڵ�ֱ��", leafSize);
	pt.put<int>("������ʶ��.�����ܶ�ʱ�����������", neighborNum);
    pt.put<double>("������ʶ��.�ϲ������߸߲�", atof(ui.lineEdit_6->text().toLocal8Bit().data()));
    pt.put<double>("������ʶ��.�ϲ������߾���", atof(ui.lineEdit_5->text().toLocal8Bit().data()));
    pt.put<double>("������ʶ��.����������", atof(ui.lineEdit_7->text().toLocal8Bit().data()));

	boost::property_tree::ini_parser::write_ini("user.ini", pt);  // �򿪶��ļ�  
}

void QPowerLineClassification::QuyuzengzhangNormalAndColor()
{
    // ��������normal+color�ָ�
    dd("QQuyuzengzhangNormal ǰcloudsize = %d", cloud_->size());
    PointCloudT::Ptr colorCloud(new PointCloudT);
    QQuyuzengzhangNormal::Fun(cloud_, colorCloud);
    dd("QQuyuzengzhangColor ǰcloudsize = %d", cloud_->size());
    QQuyuzengzhangColor::Fun(colorCloud, colorCloud);
    pcl::copyPointCloud(*colorCloud, *cloud_);
    dd("QQuyuzengzhangColor ��cloudsize = %d", cloud_->size());

    // removeSmallClass
    std::vector <pcl::PointIndices> &jlClusters = PCManage::ins().jlClusters_;
    for (auto it = jlClusters.begin(); it != jlClusters.end();)
    {
        if (it->indices.size() < 50)
        {
            it = jlClusters.erase(it);
        }
        else
        {
            ++it;
        }
    }


    UpdateShowClass("����");
    UpdateView();
}


void QPowerLineClassification::RoughExtractGroundObject()
{
    dd("������ȡ");
    if (!cloud_->size())
    {
        return;
    }

    double groundLower = 10;
    pcl::PointIndices::Ptr groundObjectErase(new pcl::PointIndices);//���̵߳Ļ��������������Ҫд����
    // ������ȡ
    double gridSize = 1;
    double heightSize = 0.5;

    // �����ά���������ֵ
    PointT min, max;
    pcl::getMinMax3D(*cloud_, min, max);
    int row = (max.x - min.x) / gridSize + 1;
    int col = (max.y - min.y) / gridSize + 1;

    // ��ʼ��ƽ������
    std::vector<std::vector<pcl::PointIndices::Ptr>>pointsArr(row, std::vector<pcl::PointIndices::Ptr>(col));
    std::vector<std::vector<pcl::PointIndices::Ptr>>groundArr(row, std::vector<pcl::PointIndices::Ptr>(col));
    PutPointCloud2Arr(cloud_, pointsArr, ground_, groundArr, row, col, min, max, gridSize);
    
#pragma omp parallel for 
    for (int i = 0; i < pointsArr.size(); ++i)
    {
        std::vector<int> eraseIndex;
        for (int j = 0; j < pointsArr[i].size(); ++j)
        {
            std::vector<int> indices;
            std::vector<float> sqr_distances;
            pcl::PointIndices::Ptr planeIndices = pointsArr[i][j];
            pcl::PointIndices::Ptr groundIndices = groundArr[i][j];
            if (!planeIndices)
            {
                continue;
            }
            if (!groundIndices)
            {
                continue;
            }
            int planeCloudSize = planeIndices->indices.size();

            // ��ȡ������ĵ���
            PointCloudT::Ptr ground(new PointCloudT);
            pcl::ExtractIndices<PointT> extract1;
            extract1.setInputCloud(ground_);
            extract1.setIndices(groundIndices);
            extract1.filter(*ground);

            PointCloudT planeCloud;
            extract1.setInputCloud(cloud_);
            extract1.setIndices(planeIndices);
            extract1.filter(planeCloud);


            // ��ȡ�������Χ����Ϣ��ÿ�������ٰ�z�������
            PointT min, max;
            getMinMax3D(planeCloud, min, max);
            float sub_z = max.z - min.z;
            int heightLevel = sub_z / heightSize + 1;
            

            // �ռ�����kdtree
            pcl::KdTreeFLANN<PointT> kdtree;
            kdtree.setInputCloud(ground);

            // �����߶�����ֱ��ͼ
            std::set<int> heighthistset;
            for (int k = 0; k < planeCloudSize; ++k)
            {
                int index = (planeCloud[k].z - min.z) / heightSize;
                heighthistset.insert(index);
            }

            // �����е��ռ�ȴ���0.8�����Ͽ�������ľ���ߵ��Ҳ�п���������
            int histSize = heighthistset.size();
            
            if (histSize >= heightLevel*0.8)
            {
                if (IsLowerClass(planeCloud, kdtree, groundLower/*, groundLnterval, aabbHeight*/))
                {
                    //dd("ɾ����ظ߶Ȳ���10��,����%f", histSize/(double)heightLevel);
                    eraseIndex.insert(eraseIndex.end(), planeIndices->indices.begin(), planeIndices->indices.end());
                }
            }
            else if (histSize < heightLevel*0.7)// ����߶ȿռ�����0.8����
            {
                indices.clear();
                sqr_distances.clear();
                // ��͵������С��1��
                if (kdtree.radiusSearch(min, 1, indices, sqr_distances))
                {
                    if (sub_z > 10)//���Ҹ߶ȴ���10��
                    {
                        double threshold = -std::numeric_limits<double>::max();
                        double curHeight = threshold;
                        double preHeight;
                        bool bfirst = true;
                        for each (int var in heighthistset)
                        {
                            curHeight = var* heightSize;
                            if (true == bfirst)
                            {
                                bfirst = false;
                            }
                            else if (curHeight - preHeight > heightSize) //�ҵ���һ���߲����2�׵ķ�Χ��Ϊ��ֵ
                            {
                                threshold = curHeight;
                                break;
                            }
                            preHeight = curHeight;
                        }

                        threshold += min.z;
                        if (min.z < threshold && threshold-min.z < sub_z*0.5)//��ֵ-��͵�<һ��
                        {
                            //double threshold = Otsu(hist) * heightSize + min.z;
                            dd("������ȡ������[%d][%d]����ߵ�=%f����͵�=%f����ظ߶�%f����ֵ%f���߲�%f������%f", i, j, max.z, min.z
                                , max.z - ground_->at(indices[0]).z, threshold, (max.z - min.z), threshold / (max.z - min.z));

                            for (std::vector<int>::iterator it = planeIndices->indices.begin(); it != planeIndices->indices.end(); ++it)
                            {
                                if (cloud_->at(*it).z < threshold)
                                {
                                    eraseIndex.push_back(*it);
                                }
                            }
                        }
                    }
                   }
            } 
        }
#pragma omp critical  
        groundObjectErase->indices.insert(groundObjectErase->indices.end(), eraseIndex.begin(), eraseIndex.end());
    }

    pcl::ExtractIndices<PointT> extract1;
    extract1.setInputCloud(cloud_);
    extract1.setIndices(groundObjectErase);


    PointCloudT::Ptr tmpcloud(new PointCloudT);
    extract1.filter(*tmpcloud);
    groundObjects_.insert(groundObjects_.begin(), tmpcloud);//�������ĵ����ر�󣬺������Ĳ�ͬ���ǾͲ嵽��һ��
    extract1.setNegative(true);
     extract1.filter(*cloud_);
}

void QPowerLineClassification::RecoverGround(PointCloudT::Ptr ground, PointCloudT::Ptr outCloud)
{
    if (!ground->size())
    {
        dd("����Ϊ�գ�����");
        return;
    }

    pcl::PointIndices::Ptr groundErase(new pcl::PointIndices);
    std::vector<pcl::PointIndices::Ptr> towerIndies;
    // ������ȡ
    double gridSize = 20;
    double heightSize = 1;

    // �����ά���������ֵ
    PointT min, max;
    pcl::getMinMax3D(*ground, min, max);
    int row = (max.x - min.x) / gridSize + 1;
    int col = (max.y - min.y) / gridSize + 1;

    // ��ʼ��ƽ������
    std::vector<std::vector<pcl::PointIndices::Ptr>>pointsArr(row, std::vector<pcl::PointIndices::Ptr>(col));
    PutPointCloud2Arr(ground, pointsArr, row, col, min, max, gridSize);

    
    // ����ÿһ������
#pragma omp parallel for
    for (int i = 0; i < pointsArr.size(); ++i)
    {
        std::vector<int> tempIndices;
        for (int j = 0; j < pointsArr[i].size(); ++j)
        {
            pcl::PointIndices::Ptr planeIndices = pointsArr[i][j];
           
            if (!planeIndices)
            {
               // dd("��������[%d][%d]ƽ������Ϊ��", i, j);
                continue;
            }

            // ׼����������
            PointCloudT planeCloud;
            pcl::ExtractIndices<PointT> extract1;
            extract1.setInputCloud(ground_);
            extract1.setIndices(planeIndices);
            extract1.filter(planeCloud);

            // ����������Ϣ
            PointT min, max;
            getMinMax3D(planeCloud, min, max);
            int heightLevel = (max.z - min.z) / heightSize + 1;
            int planeCloudSize = planeIndices->indices.size();
            double sumzCloud = 0;

            // ͳ�Ƹ߶�ֱ��ͼռ��
            std::set<int> heighthistset;
            for (int k = 0; k < planeCloudSize; ++k)
            {
                int index = (ground->at(planeIndices->indices[k]).z - min.z) / heightSize;
                if (heighthistset.find(index) == heighthistset.end())
                {
                    sumzCloud += index;
                    heighthistset.insert(index);
                }
            }

            // �߶������е��ռ��С��0.7������Ϊ����յ㣬���и߶��Զ���ֵ����
            int histSize = heighthistset.size();
            if (histSize < heightLevel * 0.7)
            {
                std::vector<double> hist;
                for each (int var in heighthistset)
                {
                    hist.push_back((var) / sumzCloud);
                }

                double threshold = Otsu(hist) * heightSize + min.z;
                dd("�������У�����[%d][%d]����ߵ�=%f����͵�=%f����ֵ%f", i, j, max.z, min.z, threshold);

                for (std::vector<int>::iterator it = planeIndices->indices.begin(); it != planeIndices->indices.end(); ++it)
                {
                    if (ground->at(*it).z > threshold)
                    {
                        tempIndices.push_back(*it);
                    }
                }
            }
        }
#pragma omp critical 
        groundErase->indices.insert(groundErase->indices.end(), tempIndices.begin(), tempIndices.end());
    }

    PointCloudT tempCloud;
    pcl::ExtractIndices<PointT> extract1;
    extract1.setInputCloud(ground);
    extract1.setIndices(groundErase);
    extract1.filter(tempCloud);
    outCloud->insert(outCloud->end(), tempCloud.begin(), tempCloud.end());

    extract1.setNegative(true);
    extract1.filter(*ground);
}

void QPowerLineClassification::OnApply()
{
    // ׼������
    *cloud_ = *PCManage::ins().cloud_;
    ground_->clear();
    groundObjects_.clear();
    towers_.clear();
    lineInfos_.clear();
    lines_.clear();

//     PointT min, max, mid;
//     pcl::getMinMax3D(*cloud_, min, max);
//     for (int i = 0; i < cloud_->size(); ++i)
//     {
//         cloud_->at(i).x += (min.x + max.x) / 2;
//         cloud_->at(i).y += (min.y + max.y) / 2;
//         cloud_->at(i).z += (min.z + max.z) / 2;
//     }

    InitCloudInfo();

    uint pre, cur, sta;
    sta = pre = GetTickCount();
    ExtractGround();

    if (0 == ground_->size())
    {
        return;
    }

    QuyuzengzhangNormalAndColor();
    dd("����������ʱ:%dms", (cur = GetTickCount()) - pre);
    pre = cur;
 
    GroundObjectFilter();
    dd("������ȡ��ʱ:%dms", (cur = GetTickCount()) - pre);
    pre = cur;

 
  	ExtractTower();
    dd("������ȡ��ʱ:%dms", (cur = GetTickCount()) - pre);
    pre = cur;
 
    ExtractPowerLine();
    dd("��������ȡ��ʱ:%dms", (cur = GetTickCount()) - pre);
    pre = cur;

    dd("����ʱ:%d", (cur = GetTickCount()) - sta);
}

double GetDistance3d(PointT &pt1, PointT &pt2)
{
	return sqrt(pow((pt1.x - pt2.x), 2) + pow((pt1.y - pt2.y), 2) + pow((pt1.z - pt2.z), 2));
}

double GetDistance2d(double x1, double y1, double x2, double y2)
{
	return sqrt(pow((x1 - x2), 2) + pow((y1 - y2), 2));
}

double GetDistance2d(PointT &pt1, PointT &pt2)
{
    return sqrt(pow((pt1.x - pt2.x), 2) + pow((pt1.y - pt2.y), 2));
}

double GetDistance2d(const vec &pt1, const vec &pt2)
{
    return sqrt(pow((pt1.x - pt2.x), 2) + pow((pt1.y - pt2.y), 2));
}

double variance(const std::vector<double> &resultSet, double *arv = nullptr)
{
    double sum = std::accumulate(std::begin(resultSet), std::end(resultSet), 0.0);
    double mean = sum / resultSet.size(); //��ֵ

    if (arv)
    {
        *arv = mean;
    }

    double accum = 0.0;
    std::for_each(std::begin(resultSet), std::end(resultSet), [&](const double d) {
        accum += (d - mean)*(d - mean);
    });

    return sqrt(accum / (resultSet.size() - 1)); //����
}

/**
*  SACSegmentation������                           
*/
 bool QPowerLineClassification::IsXyPlaneLine(PointCloudT &cloud)
 {
 	pcl::PointIndices::Ptr inliers(new pcl::PointIndices);
 	pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients());
    pcl::ModelCoefficients::Ptr coefficients1(new pcl::ModelCoefficients());
 	coefficients->values.resize(4);
 	coefficients->values[0] = coefficients->values[1] = 0;
 	coefficients->values[2] = 1.0;
 	coefficients->values[3] = 0;
 	PointCloudT cloud2d;
 	pcl::ProjectInliers<pcl::PointXYZRGB> proj;      //����ͶӰ�˲�����
 	proj.setModelType(pcl::SACMODEL_PLANE);      //���ö����Ӧ��ͶӰģ��
 	proj.setInputCloud(cloud.makeShared());                    //�����������
 	proj.setModelCoefficients(coefficients);     //����ģ�Ͷ�Ӧ��ϵ��
 	proj.filter(cloud2d);                //ִ��ͶӰ�˲��洢���cloud_projected
 
 
 	// ����һ���ָ���
 	pcl::SACSegmentation<PointT> seg;
 	// Optional
 	seg.setOptimizeCoefficients(true);
 	// Mandatory-����Ŀ�꼸����״
 	seg.setModelType(pcl::SACMODEL_LINE);
 	//�ָ�������������
 	seg.setMethodType(pcl::SAC_RANSAC);
 	//������������Ĭ��ֵΪ50��
 	seg.setMaxIterations(500);
 	//��ѯ�㵽Ŀ��ģ�͵ľ�����ֵ
 	seg.setDistanceThreshold(10);
 	//����һ��������������Ⱥ��ĸ��ʣ�Ĭ��ֵΪ0.99����
 	seg.setProbability(0.90);
 
    std::vector<double> vecx;
    std::vector<double> vecy;
    std::vector<double> vecz;
	int matchingSize = 0;
 	do 
 	{
 		//��ֱ��
 		inliers->indices.clear();
 		seg.setInputCloud(cloud2d.makeShared());
 		seg.segment(*inliers, *coefficients1);
 
 		//����ҵ�ֱ��
 		if (inliers->indices.size())
 		{
 			pcl::ExtractIndices<PointT> extract;
 			extract.setInputCloud(cloud2d.makeShared());
 			extract.setIndices(inliers);
			// ��ԭʼ������ɾ���˴μ������Ļ���
			extract.setNegative(true);
			extract.filter(cloud2d);
			matchingSize += inliers->indices.size();

            vec v;
            v.x = coefficients1->values[3];
            v.y = coefficients1->values[4];
            v.z = coefficients1->values[5];
            v.Normalize();
            vecx.push_back(v.x);
            vecy.push_back(v.y);
            vecz.push_back(v.z);
 		}
 		else
 		{
 			break;
 		}
 
 
 	} while (/*inliers->indices.size()*/true);
 

    double avex = 0;
    double avey = 0;
    double avez = 0;
    double varx = variance(vecx, &avex);
    double vary = variance(vecy, &avey);
    double varz = variance(vecz, &avez);
    dd("Ԫ�ظ�����%d�����%f,f,%f��ƽ����%f,%f,%f", vecx.size(), varx, vary, varz, avex, avey, avez);
    // ƥ�䵽����С���ܵ�����80%
	if (matchingSize < (double)cloud.size()*0.8)
  		return false;
 
 	return true;
 }

// bool QPowerLineClassification::IsXyPlaneLine(PointCloudT &cloud)
// {
// 	pcl::PointIndices::Ptr inliers(new pcl::PointIndices);
// 	pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients());
// 	pcl::ModelCoefficients::Ptr coefficients1(new pcl::ModelCoefficients());
// 	coefficients->values.resize(4);
// 	coefficients->values[0] = coefficients->values[1] = 0;
// 	coefficients->values[2] = 1.0;
// 	coefficients->values[3] = 0;
// 	PointCloudT cloud2d;
// 	pcl::ProjectInliers<pcl::PointXYZRGB> proj;      //����ͶӰ�˲�����
// 	proj.setModelType(pcl::SACMODEL_PLANE);      //���ö����Ӧ��ͶӰģ��
// 	proj.setInputCloud(cloud.makeShared());                    //�����������
// 	proj.setModelCoefficients(coefficients);     //����ģ�Ͷ�Ӧ��ϵ��
// 	proj.filter(cloud2d);                //ִ��ͶӰ�˲��洢���cloud_projected
// 
// 
// 	// ����һ���ָ���
// 	pcl::SACSegmentation<PointT> seg;
// 	// Optional
// 	seg.setOptimizeCoefficients(true);
// 	// Mandatory-����Ŀ�꼸����״
// 	seg.setModelType(pcl::SACMODEL_LINE);
// 	//�ָ�������������
// 	seg.setMethodType(pcl::SAC_RANSAC);
// 	//������������Ĭ��ֵΪ50��
// 	seg.setMaxIterations(500);
// 	//��ѯ�㵽Ŀ��ģ�͵ľ�����ֵ
// 	seg.setDistanceThreshold(0.3);
// 	//����һ��������������Ⱥ��ĸ��ʣ�Ĭ��ֵΪ0.99����
// 	seg.setProbability(0.99);
// 
// 	//��ֱ��
// 	inliers->indices.clear();
// 	seg.setInputCloud(cloud2d.makeShared());
// 	seg.segment(*inliers, *coefficients1);
// 
// 	//����ҵ�ֱ��
// 	if (inliers->indices.size())
// 	{
// 		pcl::ExtractIndices<PointT> extract;
// 		extract.setInputCloud(cloud2d.makeShared());
// 		extract.setIndices(inliers);
// 
// 
// 		// ��ԭʼ������ɾ���˴μ������Ļ���
// 		extract.setNegative(true);
// 		extract.filter(cloud2d);
// 
// 		PointCloudT cloudtemp;
// 		extract.setNegative(false);
// 		extract.filter(cloudtemp);
// 		pcl::visualization::PCLVisualizer viewer("PCL Viewer");
// 		viewer.setBackgroundColor(1, 1, 1);
// 		viewer.addPointCloud<pcl::PointXYZRGB>(cloudtemp.makeShared(), "cloud");
// 		viewer.addPointCloud<pcl::PointXYZRGB>(cloud.makeShared(), "scloud");
// 
// 		//viewer.addCoordinateSystem(1.0);
// 		//	viewer.initCameraParameters();
// 		while (!viewer.wasStopped())
// 		{
// 			viewer.spinOnce();
// 		}
// 	}
// 
// 
// 	if (inliers->indices.size() / (double)cloud.size() > 0.5)
// 		return false;
// 
// 	return true;
//  }
// #include <pcl/sample_consensus/sac_model_line.h>
// #include <pcl/sample_consensus/ransac.h>
// bool QPowerLineClassification::IsXyPlaneLine(PointCloudT &cloud)
// {
// 	pcl::PointIndices::Ptr inliers(new pcl::PointIndices);
// 	pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients());
// 	pcl::ModelCoefficients::Ptr coefficients1(new pcl::ModelCoefficients());
// 	coefficients->values.resize(4);
// 	coefficients->values[0] = coefficients->values[1] = 0;
// 	coefficients->values[2] = 1.0;
// 	coefficients->values[3] = 0;
// 	PointCloudT cloud2d;
// 	pcl::ProjectInliers<pcl::PointXYZRGB> proj;      //����ͶӰ�˲�����
// 	proj.setModelType(pcl::SACMODEL_PLANE);      //���ö����Ӧ��ͶӰģ��
// 	proj.setInputCloud(cloud.makeShared());                    //�����������
// 	proj.setModelCoefficients(coefficients);     //����ģ�Ͷ�Ӧ��ϵ��
// 	proj.filter(cloud2d);                //ִ��ͶӰ�˲��洢���cloud_projected
// 
// 
// 
// 	std::vector<int> inliers1;  //�洢���ڵ㼯�ϵĵ������������
// 	//�����������һ���Զ���
// 	pcl::SampleConsensusModelLine<pcl::PointXYZRGB>::Ptr
// 		model_l(new pcl::SampleConsensusModelLine<pcl::PointXYZRGB>(cloud2d.makeShared()));    //�����ģ�͵Ķ���
// 
// 
// 
// 	pcl::RandomSampleConsensus<pcl::PointXYZRGB> ransac(model_l);
// 		ransac.setDistanceThreshold(1.5);    //��ƽ�����С��0.01 �ĵ��Ϊ���ڵ㿼��
// 		ransac.computeModel();                   //ִ�������������
// 		ransac.getInliers(inliers1);                 //�洢�������õľ��ڵ�
// 
// 
// 		PointCloudT cloudtemp;
// 		for (int i = 0; i < inliers1.size(); ++i)
// 		{
// 			PointT &pt = cloud2d[inliers1[i]];
// 			cloudtemp.push_back(pt);
// 		}
// 		pcl::visualization::PCLVisualizer viewer("PCL Viewer");
// 		viewer.setBackgroundColor(1, 1, 1);
// 		viewer.addPointCloud<pcl::PointXYZRGB>(cloudtemp.makeShared(), "cloud");
// 		viewer.addPointCloud<pcl::PointXYZRGB>(cloud.makeShared(), "scloud");
// 
// 		while (!viewer.wasStopped())
// 		{
// 			viewer.spinOnce();
// 		}
// 
// 	return true;
// }
// 
// vec GetObbSize(OBB &obb)
// {
//     float min = 0, max = 0;
//     std::vector<float> l(3);
//     obb.ProjectToAxis(obb.axis[0], min, max);
//     l[0] = (max - min);
// 
//     min = 0, max = 0;
//     obb.ProjectToAxis(obb.axis[1], min, max);
//     l[1] = (max - min);
// 
//     min = 0, max = 0;
//     obb.ProjectToAxis(obb.axis[2], min, max);
//     l[2] = (max - min);
//     std::sort(l.begin(), l.end());
// 
//     return vec(l[2], l[1], l[0]);
// }

bool IsObbLine(PointCloudT &cloud)
{
    return true;
}

// void QPowerLineClassification::RanSacArc()
// {
// 	//inliers��ʾ��������̵ĵ� ��¼���ǵ��Ƶ����
// 	pcl::PointIndices::Ptr inliers(new pcl::PointIndices);
// 
// 	// ����һ���ָ���
// 	pcl::SACSegmentation<PointT> seg;
// 	// Optional
// 	seg.setOptimizeCoefficients(true);
// 	// Mandatory-����Ŀ�꼸����״
// 	seg.setModelType(pcl::SACMODEL_CIRCLE3D);
// 	//�ָ�������������
// 	seg.setMethodType(pcl::SAC_RANSAC);
// 	//������������Ĭ��ֵΪ50��
// 	seg.setMaxIterations(500);
// 	//��ѯ�㵽Ŀ��ģ�͵ľ�����ֵ
// 	seg.setDistanceThreshold(1.5);
// 	//����һ��������������Ⱥ��ĸ��ʣ�Ĭ��ֵΪ0.99����
// 	seg.setProbability(0.99);
// 	//�������
// 	
// 	// �ҵ������������еĻ���Ԫ��
// 	do 
// 	{
// 		inliers->indices.clear();
// 		seg.setInputCloud(cloud_);
// 		//����һ��ģ�Ͳ����������ڼ�¼���
// 		pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients);
// 
// 		seg.segment(*inliers, *coefficients);
// 
// 		PointCloudT tempCloud;
// 		pcl::ExtractIndices<PointT> extract;
// 		extract.setInputCloud(cloud_);
// 		extract.setIndices(inliers);
// 		extract.filter(tempCloud);
//  		// ��ԭʼ������ɾ���˴μ������Ļ���
//  		extract.setNegative(true);
// 		extract.filter(*cloud_);
// 
// 		
// 		// �жϴ�Բ���Ƿ���������ߵ��������������������ӵ�������������
// 		if (LikePowerLine(tempCloud))
// 		{
// 			*cloud_ += tempCloud;
// 			ui.widget->UpdateView(cloud_);
// 		}
// 	} while (inliers->indices.size());
// }

/**
*   ���aabb��Χ�е������߳���С��aabbLimit����
*/
bool QPowerLineClassification::IsAABBUnusual(PointCloudT &cloud, double aabbLimit)
{
	PointT minPt, maxPt;
	pcl::getMinMax3D(cloud, minPt, maxPt);

	double subx = maxPt.x - minPt.x;
	double suby = maxPt.y - minPt.y;
	double subz = maxPt.z - minPt.z;

	if (subx < aabbLimit && suby < aabbLimit && subz < aabbLimit)
	{
        dd("aabb��Χ���쳣���ˣ�subx=%f ��suby=%f, subz=%f", subx, suby, subz);
		return true;
	}

	return false;
}

/**
*   ��xyͶӰ������£���������������obbLimit(100)������Ϊ�����������ߵ�����
*/
bool QPowerLineClassification::IsOBBUnusual(PointCloudT &cloud, double obbLimit)
{
//     int ptct = cloud.size();
//     boost::shared_ptr<vec> points(new vec[ptct], std::default_delete<vec[]>());
//     for (int j = 0; j < cloud.size(); ++j)
//     {
//         points.get()[j] = vec(cloud[j].x, cloud[j].y, /*cloud[j].z*/0);
//     }
//     OBB obb = OBB::BruteEnclosingOBB(points.get(), ptct);
//     vec size3 = obb.Size();
// 
//     // ��������������obbLimit
//     if ((size3.x > obbLimit) + (size3.y > obbLimit) + (size3.z > obbLimit) == 2)
//     {
//         dd("obb��Χ���쳣���ˣ�xaix=%f ��yaix=%f, zaix=%f", size3.x, size3.y, size3.z);
//         return true;
//     }

    return false;
}

void QPowerLineClassification::RemoveAABBUnusual()
{
	PointCloudT::Ptr cloud = cloud_;
	std::vector <pcl::PointIndices> &jlClusters = PCManage::ins().jlClusters_;
	double aabbLimit = atof(ui.lineEdit_9->text().toLocal8Bit().data());

	std::vector <pcl::PointIndices>::iterator it;
	for (it = jlClusters.begin(); it != jlClusters.end();)
	{
		PointCloudT::Ptr curCloud(new PointCloudT);
		pcl::PointIndices::Ptr curindices(new pcl::PointIndices());
		*curindices = *it;
		pcl::ExtractIndices<PointT> extract;
		extract.setInputCloud(cloud);
		extract.setIndices(curindices);
		extract.filter(*curCloud);

        if (IsAABBUnusual(*curCloud, aabbLimit) /*|| IsOBBUnusual(curCloud)*/)
		{
			it = jlClusters.erase(it);
            groundObjects_.push_back(curCloud);
		}
		else
		{
			++it;
		}
	}
}

void QPowerLineClassification::OnGroupBoxCheck(bool cheched)
{
	QGroupBox* srcGroupBox = ui.groupBox_5;
	if (srcGroupBox)
	{
		QObjectList children = srcGroupBox->children();
		for (int i = 0; i < children.size(); ++i)
		{
			if (children[i]->isWidgetType())
			{
				QWidget* widget = qobject_cast<QWidget *>(children[i]);
				if (widget)
				{
					if (cheched)
						widget->show();
					else
						widget->hide();
				}
			}
		}
 	}
}

void QPowerLineClassification::UpdateMainView()
{
	QRendView* ins = QRendView::MainRendView();

	pcl::copyPointCloud(*cloud_, *PCManage::ins().cloud_);
	ins->UpdateView();
}

// bool QPowerLineClassification::LikePowerLine(LineInfo &line)
// {
//     int maxAxis = GetXyMaxAsix(line.sta, line.end);
//     float axismin = line.sta[maxAxis];
//     float axismax = line.end[maxAxis];
// 
//     float curZ = 0;
//     float preZ = 0;
//     bool forFirst = true;
// 
//     int ptSize = line.pts.size();
//     int halfSize = ptSize / 2;
//     int errpt = 0;
//     for (int i = 0; i < ptSize; ++i)
//     {
//         vec pt = line.pts[i];
// 
// 
//         curZ = pt.z;
//         if (forFirst)
//         {
//             forFirst = false;
//         }
//         else
//         {
//             if (curZ - preZ > 5)
//                  return false;
//         }
//         preZ = curZ;
//         //if (pt[maxAxis] - axismin < axismax - pt[maxAxis]) //�����������յ������Ƚ�
//         if (i <= halfSize)
//         {
//             vec subv = pt - line.sta;
//             subv.z = 0;
//             subv.Normalize();
//             if (!IsSimilarityVec(subv, line.v, 0.3))
//             {
//                 // dd("��%d���㵽�յ�������%f,%f,%f��������������%f,%f,%f",i, subv.x, subv.y, subv.z, line.v.x, line.v.y, line.v.z);
//                 // return false;
//                 ++errpt;
//             }
//         }
//         else//���յ��������㻭�����Ƚ�
//         {
//             vec subv = line.end - pt;
//             subv.z = 0;
//             subv.Normalize();
//             if (!IsSimilarityVec(subv, line.v, 0.3))
//             {
//                 //dd("��%d��������������%f,%f,%f��������������%f,%f,%f", i, subv.x, subv.y, subv.z, line.v.x, line.v.y, line.v.z);
//                 //return false;
//                 ++errpt;
//             }
//         }
//     }
// 
//     if (errpt / (double)line.pts.size() > 0.2)
//     {
//         return false;
//     }
// 
//     return true;
//  }

bool QPowerLineClassification::LikePowerLine(LineInfo &line)
{
    LineInfo::Asix maxAxis = line.maxAsix;
    if (line.sta.Distance(line.end) < 2)
    {
        return false;
    }

    float yerrOffset = 1.0f;
    float zerrOffset = 0.5f;
    int ptSize = line.pts.size();
    int errpt = 0;
    for (int i = 0; i < ptSize; ++i)
    {
        vec pt = line.pts[i] - info_.center;
        if (LineInfo::X == maxAxis)
        {
            if (abs(line.fit.getY(pt.x) - pt.y) > yerrOffset || abs(line.fit_z.getY(pt.x) - pt.z) > zerrOffset)
            {
                ++errpt;
            }
        }
        else
        {
            if (abs(line.fit.getY(pt.y) - pt.x) > yerrOffset || abs(line.fit_z.getY(pt.y) - pt.z) > zerrOffset)
            {
                ++errpt;
            }
        }
    }
    if (errpt / (double)ptSize > 0.1)
    {
        return false;
    }

    return true;
}

// ��ȡ������
void QPowerLineClassification::ExtractPowerLine()
{
    // ŷʽ�ָ��û�������߸��ݿռ��ϵ�ֿ�
    QOushifenge::Fun(cloud_);
    lines_.clear();

    // ��ȡ��ǰ��������
    std::vector <pcl::PointIndices>& juClusters = PCManage::ins().jlClusters_;
    pcl::PointIndices::Ptr extractIndices(new pcl::PointIndices);
    int jlclassCount = juClusters.size();

    // �ж�ÿ�������Ƿ���������ߵ��ص�
#pragma omp parallel for
    for (int i = 0; i < jlclassCount; ++i)
    {
        // ��ȡ������
        PointCloudT::Ptr lineCloud(new PointCloudT);
        pcl::PointIndices::Ptr indices(new pcl::PointIndices);
        *indices = juClusters[i];
        pcl::ExtractIndices<PointT> extract;
        extract.setInputCloud(cloud_);
        extract.setIndices(indices);
        extract.filter(*lineCloud);
        // �����������Ϣ
        boost::shared_ptr<LineInfo> line(new LineInfo);
        CalcLineInfo(lineCloud, *line);

        // �Ƿ���������ߵ��ص�
        if (LikePowerLine(*line)/*true*/)
        {
            // �洢����������
#pragma omp critical
            {
                lineInfos_.push_back(line);
                lines_.push_back(lineCloud);
                extractIndices->indices.insert(extractIndices->indices.begin(), indices->indices.begin(), indices->indices.end());
            }
            //dd("�ǵ�����,ʣ��ƽ����%f,���������%f,�ع�ƽ����%f,zʣ��ƽ����%f,z���������%f,z�ع�ƽ����%f", line->fit.getSSE() / line->pts.size(), line->fit.getRMSE() / line->pts.size(), line->fit.getSSR() / line->pts.size(), line->fit_z.getSSE() / line->pts.size(), line->fit_z.getRMSE() / line->pts.size(), line->fit_z.getSSR() / line->pts.size());
        }
        else
        {
            //dd("���ǵ�����,ʣ��ƽ����%f,���������%f,�ع�ƽ����%f,zʣ��ƽ����%f,z���������%f,z�ع�ƽ����%f",line->fit.getSSE() / line->pts.size(), line->fit.getRMSE() / line->pts.size(), line->fit.getSSR() / line->pts.size(), line->fit_z.getSSE() / line->pts.size(), line->fit_z.getRMSE() / line->pts.size(), line->fit_z.getSSR() / line->pts.size());
        }
    }

    // ���ǵ����ߵĵ�ӵ�ǰ�����Ƴ�
    pcl::ExtractIndices<PointT> extract;
    extract.setInputCloud(cloud_);
    extract.setIndices(extractIndices);
    extract.setNegative(true);
    extract.filter(*cloud_);

    LineConnect();
     for (int i = 0; i < lineInfos_.size(); ++i)
     {
         AddTube(lineInfos_[i]->fit_pts,0.3);
     }
     
     ClearShowClass();
     //SetShowClass("������", true);
     ShowTowerLines();
}

void QPowerLineClassification::TooNearCheck()
{
    uint cur, sta;
    sta = GetTickCount();
    

    RemoveViewBalls();
    balls_.clear();
    // �ж�ÿ���������͵��淶ΧK���Ƿ��н���������������������������Ľ���
    // ������������� ���Կ����Ƚ�����
    double K = ui.lineEdit_7->text().toDouble();
    double leafSize = 1.5;
    int intersectCount = 0;
    uchar labr = 255, labb = 0, labg = 0;

    std::vector<int> sectionBeginSet;
    int stepindex = 0;
    PointCloudT::Ptr groundObject(new PointCloudT);

    for (int i = 0; i < groundObjects_.size(); ++i)
    {
        sectionBeginSet.push_back(stepindex);
        groundObject->insert(groundObject->end(), groundObjects_[i]->begin(), groundObjects_[i]->end());
        stepindex += groundObjects_[i]->size();
    }


    // ����ÿһ����
#pragma omp parallel for
    for (int i = 0; i < lines_.size(); ++i)
    {
        pcl::KdTreeFLANN<PointT> groundkdtree;
        groundkdtree.setInputCloud(ground_);
        pcl::KdTreeFLANN<PointT> objectkdtree;
        objectkdtree.setInputCloud(groundObject);

        PointCloudT::Ptr line = lines_[i];
        PointCloudT::Ptr serachLine(new PointCloudT);

        pcl::VoxelGrid<PointT> grid;
        grid.setLeafSize(leafSize, leafSize, leafSize);
        grid.setInputCloud(line);
        grid.filter(*serachLine);

        int serachNum = 0;
        std::vector<int> groundindices;
        std::vector<float> groundsqr_distances;
        std::vector<int> objectindices;
        std::vector<float> objectsqr_distances;


        int maxSerachNum = -std::numeric_limits<int>::max();
        PointT maxSerachPt;
        std::vector<int> maxgroundindices;
        std::vector<int> maxobjectindices;

        // �õ��˸��ߵ�����ཻ��Χ��
        maxSerachNum = 0;
        for (int j = 0; j < serachLine->size(); ++j)
        {
            serachNum = groundkdtree.radiusSearch(serachLine->at(j), K, groundindices, groundsqr_distances) + objectkdtree.radiusSearch(serachLine->at(j), K, objectindices, objectsqr_distances);
            if (serachNum)
            {
                if (maxSerachNum < serachNum)
                {
                    maxSerachNum = serachNum;
                    maxSerachPt = serachLine->at(j);
                    maxgroundindices = groundindices;
                    maxobjectindices = objectindices;
                }
            }
        }

        // ����ཻ��
        if (maxSerachNum > intersectCount)
        {
            {// ��������ײ���
                pcl::KdTreeFLANN<PointT> linekdtree;
                std::vector<int> lineindices;
                std::vector<float> linesqr_distances;
                linekdtree.setInputCloud(line);
                // ��鵽��ײ���˸������߱��
                if (linekdtree.radiusSearch(maxSerachPt, K, lineindices, linesqr_distances))
                {
                    for (int k = 0; k < lineindices.size(); ++k)
                    {
                        PointT &pt = line->at(lineindices[k]);
                        pt.r = labr;
                        pt.g = labg;
                        pt.b = labb;
                    }
                }
                CollisionBall c;
                c.cen = maxSerachPt;
                c.radiu = K;
                c.id = (QStringLiteral("ball") + QString::number(i) /*+ QStringLiteral("point") + QString::number(maxSerachPtIndex)*/).toLocal8Bit().data();
#pragma omp critical  
                balls_.push_back(c);
                dd("��⵽����:%s", c.id.c_str());
            }

            // ����͵�����ײ���
            for (int k = 0; k < maxgroundindices.size(); ++k)
            {
#pragma omp critical  
                {
                    ground_->at(maxgroundindices[k]).r = 0;
                    ground_->at(maxgroundindices[k]).g = 0;
                    ground_->at(maxgroundindices[k]).b = 255;
                }
            }

            int session = 0;
            int sessionnum = 0;
            for (int k = 0; k < maxobjectindices.size(); ++k)
            {
                // �����������飬�ж�maxobjectindices[k]�ǵڼ����������������
                // ֮���Կ�������������Ϊmaxobjectindices��һ��˳�����е������������groundObject��һ������ĵ���
                for (int n = sectionBeginSet.size() - 1; n >= 0; --n)
                {
                    if (sectionBeginSet[n] <= maxobjectindices[k])
                    {
                        session = n;
                        sessionnum = sectionBeginSet[n];
                        break;
                    }
                }
                PointT &pt = groundObjects_[session]->at(maxobjectindices[k] - sessionnum);
#pragma omp critical  
                {
                    pt.r = 255;
                    pt.g = 0;
                    pt.b = 0;
                }
            }
        }
    }

    UpdateShowClass("��ײ��");
    ShowTooNearCheck();
}


// void QPowerLineClassification::TooNearCheck()
// {
//     RemoveViewBalls();
//     balls_.clear();
//     // �ж�ÿ���������͵��淶ΧK���Ƿ��н���������������������������Ľ���
//     // ������������� ���Կ����Ƚ�����
//     double K = ui.lineEdit_7->text().toDouble();
//     double leafSize = 1.5;
//     int intersectCount = 0;
//     uchar labr = 255, labb = 0, labg = 0;
// 
//     std::vector<int> sectionBeginSet;
//     int stepindex = 0;
//     PointCloudT::Ptr groundObject(new PointCloudT);
// 
//     for (int i = 0; i < groundObjects_.size(); ++i)
//     {
//         sectionBeginSet.push_back(stepindex);
//         groundObject->insert(groundObject->end(), groundObjects_[i]->begin(), groundObjects_[i]->end());
//         stepindex += groundObjects_[i]->size();
//     }
// 
//     std::map<int, std::set<int>> g_lineset;
//     std::set<int> g_groundset;
//     std::map<int, std::set<int>> g_objectsets;
// 
// 
//     // ����ÿһ����
// #pragma omp parallel for
//     for (int i = 0; i < lines_.size(); ++i)
//     {
//         pcl::KdTreeFLANN<PointT> groundkdtree;
//         groundkdtree.setInputCloud(ground_);
//         pcl::KdTreeFLANN<PointT> objectkdtree;
//         objectkdtree.setInputCloud(groundObject);
// 
//         PointCloudT::Ptr line = lines_[i];
//         PointCloudT::Ptr serachLine(new PointCloudT);
// 
//         pcl::VoxelGrid<PointT> grid;
//         grid.setLeafSize(leafSize, leafSize, leafSize);
//         grid.setInputCloud(line);
//         grid.filter(*serachLine);
// 
//         int serachNum = 0;
//         std::vector<int> groundindices;
//         std::vector<float> groundsqr_distances;
//         std::vector<int> objectindices;
//         std::vector<float> objectsqr_distances;
// 
//         std::set<int> lineset;
//         std::set<int> groundset;
//         std::map<int, std::set<int>> objectsets;
// 
//         // �õ��˸��ߵ�����ཻ��Χ��
//         for (int j = 0; j < serachLine->size(); ++j)
//         {
//             serachNum = groundkdtree.radiusSearch(serachLine->at(j), K, groundindices, groundsqr_distances) + objectkdtree.radiusSearch(serachLine->at(j), K, objectindices, objectsqr_distances);
// 
//             if (serachNum > intersectCount)
//             {
//                 {// ��������ײ���
//                     pcl::KdTreeFLANN<PointT> linekdtree;
//                     std::vector<int> lineindices;
//                     std::vector<float> linesqr_distances;
//                     linekdtree.setInputCloud(line);
//                     // ��鵽��ײ���˸������߱��
//                     if (linekdtree.radiusSearch(serachLine->at(j), K, lineindices, linesqr_distances))
//                     {
//                         for (int k = 0; k < lineindices.size(); ++k)
//                         {
//                             lineset.insert(lineindices[k]);
//                         }
//                     }
//                 }
// 
//                 // ������ײ���
//                 for (int k = 0; k < groundindices.size(); ++k)
//                 {
//                     groundset.insert(groundindices[k]);
//                 }
// 
//                 // ������ײ���
//                 int session = 0;
//                 int sessionnum = 0;
//                 for (int k = 0; k < objectindices.size(); ++k)
//                 {
//                     // �����������飬�ж�maxobjectindices[k]�ǵڼ����������������
//                     // ֮���Կ�������������Ϊmaxobjectindices��һ��˳�����е������������groundObject��һ������ĵ���
//                     for (int n = sectionBeginSet.size() - 1; n >= 0; --n)
//                     {
//                         if (sectionBeginSet[n] <= objectindices[k])
//                         {
//                             session = n;
//                             sessionnum = sectionBeginSet[n];
//                             break;
//                         }
//                     }
//                     objectsets[session].insert(objectindices[k] - sessionnum);
//                 }
//             }
//         }
// #pragma omp critical  
//         {
//             g_lineset[i].insert(lineset.begin(), lineset.end());
//             g_groundset.insert(groundset.begin(), groundset.end());
//             for (auto itt = objectsets.begin(); itt != objectsets.end(); ++itt)
//             {
//                 int session = itt->first;
//                 auto &objectItem = itt->second;
//                 g_objectsets[session].insert(objectItem.begin(), objectItem.end());
//             }
//         }
//     }
// 
//    
//     for (std::map<int, std::set<int>>::iterator itt = g_lineset.begin(); itt != g_lineset.end(); ++itt)
//     {
//         int session = itt->first;
//         auto &lineItem = itt->second;
//         for (std::set<int>::iterator it = lineItem.begin(); it != lineItem.end(); ++it)
//         {
//             PointT &pt = lines_[session]->at(*it);
//             pt.r = labr;
//             pt.g = labg;
//             pt.b = labb;
//         }
//     }
//     for (std::set<int>::iterator it = g_groundset.begin(); it != g_groundset.end(); ++it)
//     {
//         PointT &pt = ground_->at(*it);
//         pt.r = 0;
//         pt.g = 0;
//         pt.b = 255;
//     }
//     for (auto itt = g_objectsets.begin(); itt != g_objectsets.end(); ++itt)
//     {
//         int session = itt->first;
//         for (std::set<int>::iterator it = itt->second.begin(); it != itt->second.end(); ++it)
//         {
//             PointT &pt = groundObjects_[session]->at(*it);
//             pt.r = 255;
//             pt.g = 0;
//             pt.b = 0;
//         }
//     }
//     UpdateShowClass("��ײ��");
//     ShowTooNearCheck();
// }

void QPowerLineClassification::ShowTooNearCheck()
{
    UpdateShowClass("����");
    UpdateShowClass("����");
    UpdateShowClass("����");
    UpdateShowClass("������");
    UpdateShowClass("������ģ��");
    UpdateShowClass("����");
    UpdateShowClass("��ײ��");

    UpdateView();

    dd("��ײ���ɱ���");
    ui.widget->viewer_->saveScreenshot("aaaaaaaaaaa.png");
}

void QPowerLineClassification::ClearShowClass()
{
    SetShowClass("����", false);
    SetShowClass("����", false);
    SetShowClass("����", false);
    SetShowClass("������", false);
    SetShowClass("������ģ��", false);
    SetShowClass("����", false);
    SetShowClass("��ײ��", false);
}

void QPowerLineClassification::ShowTowerLines()
{
    ClearShowClass();
    SetShowClass("����", false);
    UpdateShowClass("����");
    UpdateShowClass("������");
    UpdateShowClass("������ģ��");

    UpdateView();
}

bool QPowerLineClassification::LikeTower(PointCloudT &cloud)
{
    
    return true;
}

// ���������泬��K�ף�����Ϊ���ǵ���
void QPowerLineClassification::RecoverGroundObject()
{
    int K = 5;
    pcl::KdTreeFLANN<PointT> groundkdtree;
    groundkdtree.setInputCloud(ground_);

    std::vector<bool> beraseVec(groundObjects_.size(), false);
#pragma omp parallel for
    for (int i = 0; i < groundObjects_.size(); ++i)
    {
        std::vector<int> groundindices;
        std::vector<float> groundsqr_distances;
        PointCloudT::Ptr object = groundObjects_[i];
        pcl::VoxelGrid<PointT> grid;
        grid.setLeafSize(1, 1, 1);
        grid.setInputCloud(object);
        grid.filter(*object);

        for (int j = 0; j < object->size(); ++j)
        {
            if (!groundkdtree.radiusSearch(object->at(j), K, groundindices, groundsqr_distances))
            {
                beraseVec[i] = true;
                break;
            }
        }
    }
    int step = 0;
    for (auto it = groundObjects_.begin(); it != groundObjects_.end(); ++step)
    {
        if (beraseVec[step])
        {
            //cloud_->insert(cloud_->end(), (*it)->begin(), (*it)->end());
            it = groundObjects_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

int QPowerLineClassification::GetXyMaxAsix(vec &sta, vec& end)
{
    vec v;
    if (abs(sta.x - end.x) > abs(sta.y - end.y)) //���x�Ŀ�ȴ���y�Ŀ��
    {
        return 0;
    }
    else
    {
        return 1;
    }
}


// ����������ά���������
vec QPowerLineClassification::GetMaxAxisVec(vec &sta, vec& end, bool bSwap /*= false*/)
{
    vec v;
    if (abs(sta.x - end.x) > abs(sta.y - end.y)) //���x�Ŀ�ȴ���y�Ŀ��
    {
        if (sta.x < end.x)
        {
            v = end - sta;
        }
        else// �����յ��
        {
            if (bSwap)
            {
                swapvec(sta, end);//�ã�����ȵ�ά�ȣ�С��ֵΪsta�����ֵΪend
                v = end - sta;
            }
            else
            {
                // dd("GetMaxAxisVec error");
                v = sta - end;
            }
        }
    }
    else
    {
        if (sta.y < end.y)
        {
            v = end - sta;
        }
        else
        {
            if (bSwap)
            {
                swapvec(sta, end);
                v = end - sta;
            }
            else
            {
                //dd("GetMaxAxisVec error");
                v = sta - end;
            }
        }
    }
    v.z = 0;
    v.Normalize();
    return v;
}

vec QPowerLineClassification::GetMaxAxisVec(const LineInfo &info)
{
    vec sub;
    sub = info.end - info.sta;
    //sub.z = 0;
    sub.Normalize();
    return sub;
}

void QPowerLineClassification::swapvec(vec &v1, vec &v2)
{
    vec temp = v1;
    v1 = v2;
    v2 = temp;
}

bool compX(const vec &v1, const vec &v2)
{
    return v1.x < v2.x;
}

bool compY(const vec &v1, const vec &v2)
{
    return v1.y < v2.y;
}

// void QPowerLineClassification::GetXyEndpoint(PointCloudT &cloud, LineInfo &info)
// {
//     // ���������Զ��������
//     double maxDiatance = -std::numeric_limits<double>::max();
//     int curDistance = -1;
//     int maxIndex1 = -1, maxIndex2 = -1;
//     for (int i = 0; i < cloud.size() - 1; ++i)
//     {
//         for (int j = i + 1; j < cloud.size(); ++j)
//         {
//             curDistance = GetDistance2d(cloud[i].x, cloud[i].y, cloud[j].x, cloud[j].y);
//             if (curDistance > maxDiatance)
//             {
//                 maxDiatance = curDistance;
//                 maxIndex1 = i;
//                 maxIndex2 = j;
//             }
//         }
//     }
// 
//     // ��������
//     info.pts.clear();
//     for (int i = 0; i < cloud.size(); ++i)
//     {
//         info.pts.push_back(vec(cloud[i].x, cloud[i].y, cloud[i].z/*0*/));
//     }
// 
//     // �þ�����Զ����������Ϊ���յ�
//     info.sta = vec(cloud[maxIndex1].x, cloud[maxIndex1].y, cloud[maxIndex1].z);
//     info.end = vec(cloud[maxIndex2].x, cloud[maxIndex2].y, cloud[maxIndex2].z);
// 
//     // �������������󣬼�������
//     int maxAsix = GetXyMaxAsix(info.sta, info.end);
//     if (0 == maxAsix)
//     {
//         std::sort(info.pts.begin(), info.pts.end(), compX);
//     }
//     else
//     {
//         std::sort(info.pts.begin(), info.pts.end(), compY);
//     }
// 
//     info.v = GetMaxAxisVec(info.sta, info.end, true);
// }

int GetMaxDim(PointCloudT &cloud)
{
    float minx = std::numeric_limits<float>::max();
    float miny = std::numeric_limits<float>::max();
    float maxx = -std::numeric_limits<float>::max();
    float maxy = -std::numeric_limits<float>::max();
    for (int i = 0; i < cloud.size(); ++i)
    {
        PointT &pt = cloud[i];
        if (pt.x < minx)
            minx = pt.x;
        if (pt.x > maxx)
            maxx = pt.x;
        if (pt.y < miny)
            miny = pt.y;
        if (pt.y > maxy)
            maxy = pt.y;
    }
    float subx = maxx - minx;
    float suby = maxy - miny;
    return subx > suby ? 0 : 1;
}

vec PointT2Vec(const PointT &ptt)
{
    return vec(ptt.x, ptt.y, ptt.z);
}

PointT Vec2PointT(const vec &ptv)
{
    PointT ptt;
    ptt.x = ptv.x;
    ptt.y = ptv.y;
    ptt.z = ptv.z;
    return ptt;
}

// ��ô˸��ߵ�����
void QPowerLineClassification::CalcLineInfo(PointCloudT::Ptr cloud, LineInfo &info)
{
    // װ�����е�
    info.pts.clear();
    double block = 1;

    int cloudSize = cloud->size();
    info.pts.resize(cloudSize);
    for (int i = 0; i < cloudSize; ++i)
    {
        PointT &pt = cloud->at(i);
        info.pts[i].x = pt.x;
        info.pts[i].y = pt.y;
        info.pts[i].z = pt.z;
    }
    std::vector<vec> pts = info.pts;
    for (int i = 0; i < cloudSize; ++i)
    {
        pts[i] -= info_.center;
    }

    // �õ������
    info.maxAsix = (LineInfo::Asix)GetMaxDim(*cloud);

    // ��ʼ����ϱ���
    std::vector<float> xDim(cloudSize), yDim(cloudSize), zDim(cloudSize);
    float tmpX = 0, tmpY = 0, tmpZ = 0;

    if (LineInfo::X == info.maxAsix)//x���ȴ�
    {
        std::sort(pts.begin(), pts.end(), compX);

        for (int i = 0; i < cloudSize; ++i)
        {
            xDim[i] = pts[i].x;
            yDim[i] = pts[i].y;
            zDim[i] = pts[i].z;
        }

        info.fit.linearFit(xDim, yDim);

        info.fit_z.polyfit(xDim, zDim, 2);

        info.sta.x = xDim[0];
        info.sta.y = info.fit.getY(xDim[0]);
        info.sta.z = info.fit_z.getY(xDim[0]);

        info.end.x = xDim[cloudSize - 1];
        info.end.y =info.fit.getY(xDim[cloudSize - 1]);
        info.end.z = info.fit_z.getY(xDim[cloudSize - 1]);

        info.v = GetMaxAxisVec(info);
        info.sta += info_.center;
        info.end += info_.center;
        
//         for (int i = 0; ((info.v*(block*i)).x + info.sta.x <= info.end.x + block); ++i)
//         {
//             vec v = info.sta + info.v*(block*i);
//             v.z = info.fit_z.getY(v.x- info_.center.x) + info_.center.z;
//             info.fit_pts.push_back(v);
//         }

        int fitsize = ceil(info.end.Distance(info.sta) / block) + 1;
        info.fit_pts.resize(fitsize);
        info.fit_pts[0] = info.sta;
        for (int i = 1; i < fitsize; ++i)
        {
            info.fit_pts[i] = info.sta + info.v*(block*i);
            info.fit_pts[i].y = info.fit.getY(info.fit_pts[i].x - info_.center.x) + info_.center.y;
            info.fit_pts[i].z = info.fit_z.getY(info.fit_pts[i].x - info_.center.x) + info_.center.z;
        }

//         int blocks_count = info.end.Distance(info.sta) / block + 1;
//  
// 
//          info.fit_pts.resize(blocks_count);
//          for (int i = 0; i < blocks_count; ++i)
//          {
//              info.fit_pts[i] = info.sta + info.v*(block*i);
//              info.fit_pts[i].z = info.fit_z.getY(info.fit_pts[i].x - info_.center.x) + info_.center.z;
//          }
        //info.fit_pts.push_back(info.end);
    }
    else//y���ȴ�
    {
        std::sort(pts.begin(), pts.end(), compY);

        for (int i = 0; i < cloudSize; ++i)
        {
            xDim[i] = pts[i].x;
            yDim[i] = pts[i].y;
            zDim[i] = pts[i].z;
        }

        info.fit.linearFit(yDim, xDim);
        info.fit_z.polyfit(yDim, zDim, 2);

        info.sta.x = info.fit.getY(yDim[0]);
        info.sta.y = yDim[0];
        info.sta.z = info.fit_z.getY(yDim[0]);

        info.end.x = info.fit.getY(yDim[cloudSize - 1]);
        info.end.y = yDim[cloudSize - 1];
        info.end.z = info.fit_z.getY(yDim[cloudSize - 1]);

        info.v = GetMaxAxisVec(info);
        info.sta += info_.center;
        info.end += info_.center;




//         for (int i = 0; ((info.v*(block*i)).y+info.sta.y <= info.end.y+block); ++i)
//         {
//             vec v = info.sta + info.v*(block*i);
//             v.z = info.fit_z.getY(v.y - info_.center.y) + info_.center.z;
//             info.fit_pts.push_back(v);
//         }

        int fitsize = ceil(info.end.Distance(info.sta) / block)+1;
        info.fit_pts.resize(fitsize);
        info.fit_pts[0] = info.sta;
        for (int i = 1; i < fitsize; ++i)
        {
            info.fit_pts[i] = info.sta + info.v*(block*i);
            info.fit_pts[i].x = info.fit.getY(info.fit_pts[i].y - info_.center.y) + info_.center.x;
            info.fit_pts[i].z = info.fit_z.getY(info.fit_pts[i].y - info_.center.y) + info_.center.z;
        }


//         int blocks_count = info.end.Distance(info.sta) / block + 1;
//          info.fit_pts.resize(blocks_count);
//          for (int i = 0; i < blocks_count; ++i)
//          {
//              info.fit_pts[i] = info.sta + info.v*(block*i);
//              info.fit_pts[i].z = info.fit_z.getY(info.fit_pts[i].y - info_.center.y) + info_.center.z;
//           }
//          vec t = info.end;
//          t.z = info.fit_z.getY(t.y - info_.center.y) + info_.center.z;
//          dd("info.fit_pts.size-1=%f,t.z=%f", info.fit_pts[info.fit_pts.size() - 1].y, t.y);
//          info.fit_pts.push_back(t);
    }
}

double round(double number, unsigned int bits) {
    stringstream ss;
    ss << fixed << setprecision(bits) << number;
    ss >> number;
    return number;
}

void QPowerLineClassification::AddTube(const std::vector<vec> &pts, const float radiu)
{
    int pts_size = pts.size();
    if (pts_size < 2)
        return;
    vtkSmartPointer<vtkPoints> points(vtkPoints::New());

    vtkSmartPointer<vtkPolyLine> polyline =
        vtkSmartPointer<vtkPolyLine>::New();
    vtkSmartPointer<vtkCellArray> polyLines =   //��Ԫ��  
        vtkSmartPointer<vtkCellArray>::New();
    const int pointNum = pts_size;
    polyline->GetPointIds()->SetNumberOfIds(pointNum);
    for (int i = 0; i < pointNum; ++i)
    {
        vec v;
        vtkIdType pid[1];
        double x = pts[i].x/*round(pts[i].x, 4)*/;
        double y = pts[i].y/*round(pts[i].y, 4)*/;
        double z = pts[i].z/*round(pts[i].z, 4)*/;
        pid[0] = points->InsertNextPoint(x, y, z);
        polyline->GetPointIds()->SetId(i, pid[0]);
    }
    polyLines->InsertNextCell(polyline);
    vtkSmartPointer<vtkPolyData> polyData =
        vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(points);
    polyData->SetLines(polyLines);


    // Interpolate the scalars  
    double rad;
    vtkSmartPointer<vtkTupleInterpolator> interpolatedRadius =
        vtkSmartPointer<vtkTupleInterpolator> ::New();
    interpolatedRadius->SetInterpolationTypeToLinear();
    interpolatedRadius->SetNumberOfComponents(1);
    for (int i = 0; i < pts_size; ++i)
    {
        rad = radiu; interpolatedRadius->AddTuple(i, &rad);
    }


    // Generate the radius scalars  
    vtkSmartPointer<vtkDoubleArray> tubeRadius =
        vtkSmartPointer<vtkDoubleArray>::New();
    //unsigned int n = functionSource->GetOutput()->GetNumberOfPoints();
    unsigned int n = pts_size;
    tubeRadius->SetNumberOfTuples(n);
    tubeRadius->SetName("TubeRadius");
    double tMin = interpolatedRadius->GetMinimumT(); 
    double tMax = interpolatedRadius->GetMaximumT(); 
    double r;
    for (unsigned int i = 0; i < n; ++i)
    {
        double t = (tMax - tMin) / (n - 1) * i + tMin;
        interpolatedRadius->InterpolateTuple(1, &r);
        tubeRadius->SetTuple1(i, r);
    }

    // Add the scalars to the polydata  
    vtkSmartPointer<vtkPolyData> tubePolyData =
        vtkSmartPointer<vtkPolyData>::New();
    //tubePolyData = functionSource->GetOutput();
    tubePolyData = polyData;
    tubePolyData->GetPointData()->AddArray(tubeRadius);
    tubePolyData->GetPointData()->SetActiveScalars("TubeRadius");
    // Create the tubes  
    vtkSmartPointer<vtkTubeFilter> tuber =
        vtkSmartPointer<vtkTubeFilter>::New();
#if VTK_MAJOR_VERSION <= 5  
    tuber->SetInput(tubePolyData);
#else  
    tuber->SetInputData(tubePolyData);
#endif  
    tuber->SetNumberOfSides(50);
    tuber->SetVaryRadiusToVaryRadiusByAbsoluteScalar();

//     //--------------  
//     // Setup actors and mappers  
//     vtkSmartPointer<vtkPolyDataMapper> lineMapper =
//         vtkSmartPointer<vtkPolyDataMapper>::New();
// #if VTK_MAJOR_VERSION <= 5  
//     lineMapper->SetInput(tubePolyData);
// #else  
//     lineMapper->SetInputData(tubePolyData);
// #endif  
//     lineMapper->SetScalarRange(tubePolyData->GetScalarRange());

    vtkSmartPointer<vtkPolyDataMapper> tubeMapper =
        vtkSmartPointer<vtkPolyDataMapper>::New();
    tubeMapper->SetInputConnection(tuber->GetOutputPort());
    tubeMapper->SetScalarRange(tubePolyData->GetScalarRange());

//     vtkSmartPointer<vtkActor> lineActor = vtkSmartPointer<vtkActor>::New();
//     lineActor->SetMapper(lineMapper);
    vtkSmartPointer<vtkActor> tubeActor = vtkSmartPointer<vtkActor>::New();
    tubeActor->SetMapper(tubeMapper);
    tubeActor->GetProperty()->SetOpacity(0.2);
    

    vtkRenderer *render = ui.widget->viewer_->getRenderWindow()->GetRenderers()->GetFirstRenderer();
    //render->AddActor(lineActor);
    render->AddActor(tubeActor);
    line_models_.insert(tubeActor);
}

bool QPowerLineClassification::IsSimilarityVec(vec &v1, vec &v2, double error/* = 0.15*/)
{
    return (abs(v1.x - v2.x) < error
        &&abs(v1.y - v2.y) < error
        /*&&abs(v1.z - v2.z) < 0.1*/);
}

bool QPowerLineClassification::IsSimilarityVec(vec &pt1, vec &pt2, vec& sumvec)
{
    vec sub = GetMaxAxisVec(pt1, pt2);
    return (abs(sub.x - sumvec.x) < 0.15
        &&abs(sub.y - sumvec.y) < 0.15
        /*&&abs(sub.z - sumvec.z) < 0.1*/);
}
// 
// // �ĳ�lineinfo �ж�����������ý���Ȼ���ж��������������������һ���ߵ�����һ�£���� һ������Ϊ��������
// bool QPowerLineClassification::IsConnectLine(LineInfo &l1, LineInfo &l2)
// {
//     double subZ = ui.lineEdit_6->text().toDouble();
//     double dis = ui.lineEdit_5->text().toDouble();
//     vec sum = (l1.v + l2.v) / 2;
//     double s = 0;
//     double d = 0;
//     if ((s = abs(l1.sta.z - l2.sta.z)) < subZ && (d = GetDistance2d(l1.sta, l2.sta)) < dis)
//     {
//         if (IsSimilarityVec(l1.sta, l2.sta, sum))
//         {
//             //dd("�߲�=%f��xy����=%f", s, d);
//             return true;
//         }
//     }
//     else if ((s = abs(l1.sta.z - l2.end.z)) < subZ && (d = GetDistance2d(l1.sta, l2.end)) < dis)
//     {
//         if (IsSimilarityVec(l1.sta, l2.end, sum))
//         {
//             //dd("�߲�=%f��xy����=%f", s, d);
//             return true;
//         }
//     }
//     else if ((s = abs(l1.end.z - l2.sta.z)) < subZ && (d = GetDistance2d(l1.end, l2.sta)) < dis)
//     {
//         if (IsSimilarityVec(l1.end, l2.sta, sum))
//         {
//             //dd("�߲�=%f��xy����=%f", s, d);
//             return true;
//         }
//     }
//     else if ((s = abs(l1.end.z - l2.end.z)) < subZ && (d = GetDistance2d(l1.end, l2.end)) < dis)
//     {
//         if (IsSimilarityVec(l1.end, l2.end, sum))
//         {
//             //dd("�߲�=%f��xy����=%f", s, d);
//             return true;
//         }
//      }
// 
//     return false;
// }

// �����ӽ��������һ�£�����һ�£�zֵ�ӽ�������ӽ���
// bool QPowerLineClassification::IsConnectLine(const LineInfo &l1, const LineInfo &l2)
// {
//     if (l1.maxAsix != l2.maxAsix)
//         return false;
//     LineInfo::Asix maxAsix = l1.maxAsix;
//     if (LineInfo::X == maxAsix)
//     {
//         if (!((l1.sta.x > l2.end.x) || (l2.sta.x > l1.end.x)))
//             return false;
//     }
//     else if (LineInfo::Y == maxAsix)
//     {
//         if (!((l1.sta.y > l2.end.y) || (l2.sta.y > l1.end.y)))
//             return false;
//     }
// 
//     
//     double subZ = ui.lineEdit_6->text().toDouble();
//     double dis = ui.lineEdit_5->text().toDouble();
//     double fitOff = 0.5f;
// 
//     // �ж�������ֱ������Ƿ�ӽ�
//     if (abs(l1.sta.z - l2.end.z) < subZ &&  GetDistance2d(l1.sta, l2.end) < dis)
//     {
//         if (LineInfo::X == maxAsix)
//         {
//             float l1fit2y = l1.fit.getY(l2.end.x - info_.center.x) + info_.center.y;
//             dd("maxAsix:%d, fity=%f, l2y=%f", LineInfo::X, l1fit2y, l2.end.y);
//             if (abs(l1fit2y - l2.end.y) < fitOff)
//             {
//                 return true;
//             }
//         }
//         else if (LineInfo::Y == maxAsix)
//         {
//             float l1fit2x = l1.fit.getY(l2.end.y - info_.center.y) + info_.center.x;
//             dd("maxAsix:%d, fitx=%f, l2x=%f", LineInfo::Y, l1fit2x, l2.end.x);
//             if (abs(l1fit2x - l2.end.x) < fitOff)
//             {
//                 return true;
//             }
//         }
//     }
//     else if (abs(l1.end.z - l2.sta.z) < subZ && GetDistance2d(l1.end, l2.sta) < dis)
//     {
//         if (LineInfo::X == maxAsix)
//         {
//             float l1fit2y = l1.fit.getY(l2.sta.x - info_.center.x) + info_.center.y;
//             dd("maxAsix:%d, fity=%f, l2y=%f", LineInfo::X, l1fit2y, l2.sta.y);
//             if (abs(l1fit2y - l2.sta.y) < fitOff)
//             {
//                 return true;
//             }
//         }
//         else if (LineInfo::Y == maxAsix)
//         {
//             float l1fit2x = l1.fit.getY(l2.sta.y - info_.center.y) + info_.center.x;
//             dd("maxAsix:%d, fitx=%f, l2x=%f", LineInfo::Y, l1fit2x, l2.sta.x);
//             if (abs(l1fit2x - l2.sta.x) < fitOff)
//             {
//                 return true;
//             }
//         }
//     }
// 
// 
//     return false;
// }

// �����ӽ��������һ�£�����һ�£�zֵ�ӽ�������ӽ���
bool QPowerLineClassification::IsConnectLine(const LineInfo &l1, const LineInfo &l2)
{
    if (l1.maxAsix != l2.maxAsix)
        return false;
    LineInfo::Asix maxAsix = l1.maxAsix;
    if (LineInfo::X == maxAsix)
    {
        if (!((l1.sta.x - l2.end.x > -2) || (l2.sta.x - l1.end.x> -2)))
            return false;
    }
    else if (LineInfo::Y == maxAsix)
    {
        if (!((l1.sta.y - l2.end.y> -2) || (l2.sta.y - l1.end.y > -2)))
            return false;
    }

    double subZ = ui.lineEdit_6->text().toDouble();
    double dis = ui.lineEdit_5->text().toDouble();
    double yfitOff = 0.5f;
    double zfitOff = 0.5f;

    // �ж�������ֱ������Ƿ�ӽ�
    if (abs(l1.sta.z - l2.end.z) < subZ &&GetDistance2d(l1.sta, l2.end) < dis)
    {
        if (LineInfo::X == maxAsix)
        {
            float l1fit2y = l1.fit.getY(l2.end.x - info_.center.x) + info_.center.y;
            float  l1fit2z = l1.fit_z.getY(l2.end.x - info_.center.x) + info_.center.z;
            dd("maxAsix:%d, fity=%f, l2y=%f", LineInfo::X, l1fit2y, l2.end.y);
            if (abs(l1fit2y - l2.end.y) < yfitOff && abs(l1fit2z - l2.end.z) < zfitOff)
            {
                return true;
            }
        }
        else if (LineInfo::Y == maxAsix)
        {
            float l1fit2x = l1.fit.getY(l2.end.y - info_.center.y) + info_.center.x;
            float l1fit2z = l1.fit_z.getY(l2.end.y - info_.center.y) + info_.center.z;
            dd("maxAsix:%d, fitx=%f, l2x=%f", LineInfo::Y, l1fit2x, l2.end.x);
            if (abs(l1fit2x - l2.end.x) < yfitOff && abs(l1fit2z - l2.end.z) < zfitOff)
            {
                return true;
            }
        }
    }
    else if (abs(l1.end.z - l2.sta.z)< subZ  && GetDistance2d(l1.end, l2.sta) < dis)
    {
        if (LineInfo::X == maxAsix)
        {
            float l1fit2y = l1.fit.getY(l2.sta.x - info_.center.x) + info_.center.y;
            float l1fit2z = l1.fit_z.getY(l2.sta.x - info_.center.x) + info_.center.z;
            dd("maxAsix:%d, fity=%f, l2y=%f", LineInfo::X, l1fit2y, l2.sta.y);
            if (abs(l1fit2y - l2.sta.y) < yfitOff && abs(l1fit2z - l2.sta.z) < zfitOff)
            {
                return true;
            }
        }
        else if (LineInfo::Y == maxAsix)
        {
            float l1fit2x = l1.fit.getY(l2.sta.y - info_.center.y) + info_.center.x;
            float l1fit2z = l1.fit_z.getY(l2.sta.y - info_.center.y) + info_.center.z;
            dd("maxAsix:%d, fitx=%f, l2x=%f", LineInfo::Y, l1fit2x, l2.sta.x);
            if (abs(l1fit2x - l2.sta.x) < yfitOff && abs(l1fit2z - l2.sta.z) < zfitOff)
            {
                return true;
            }
        }
    }
    return false;
}


// ����������
// ����1�����ÿ���ߵĶ˵㣬������������߶Ⱥ;�����������Ƚϣ�����������ӵ��������������ߵ�������ֵ��������
void QPowerLineClassification::LineConnect()
{
    dd("�ϲ�������");
    std::vector<boost::shared_ptr<LineInfo>> &lineInfos = lineInfos_;
    for (int i = 0; i < lines_.size() - 1; ++i)
    {
        for (int j = i + 1; j < lines_.size(); ++j)
        {
            if (!lines_[i]->size())
                 break;
            if (!lines_[j]->size())
                continue;
           
            boost::shared_ptr<LineInfo> &infoI = lineInfos[i];
            boost::shared_ptr<LineInfo> &infoJ = lineInfos[j];

            // �����ӽ�������
            if (IsConnectLine(*infoI, *infoJ))
            {
                 dd("�ϲ�������[%d]�������[%d]", i, j);
                 lines_[j]->insert(lines_[j]->end(), lines_[i]->begin(), lines_[i]->end());
                 CalcLineInfo(lines_[j], *infoJ);
                 setColors(*lines_[j], rand() % 256, rand() % 256, rand() % 256);
                 lines_[i]->clear();
                 infoI->pts.clear();
            }
        }
    }
     // ɾ���յģ�ȥ���ظ���
    auto itt = lineInfos.begin();
    for (auto it = lines_.begin(); it != lines_.end();)
     {
        if ((*it)->size() == 0)
        {
            it = lines_.erase(it);
            itt = lineInfos.erase(itt);
        }
            
         else
         {
             ++it;
             ++itt;
         }
     }
}

void QPowerLineClassification::RemoveViewClouds(QString name)
{
    int i = 0;
    while (true)
    {
        QString index = name + QString::number(i);
        if (ui.widget->viewer_->contains(index.toLocal8Bit().data()))
        {
            ui.widget->viewer_->removePointCloud(index.toLocal8Bit().data());
        }
        else
        {
            break;
        }
        ++i;
    }
}

void QPowerLineClassification::RemoveViewBalls()
{
    for (int i = 0; i < balls_.size(); ++i)
    {
        ui.widget->viewer_->removeShape(balls_[i].id);
    }
}

void QPowerLineClassification::SetShowClass(std::string classname, bool checked/* = true*/)
{
    if ("����" == classname)
    {
        if (ui.checkBox->isChecked() != checked)
        {
            ui.checkBox->setChecked(checked);
            SetShowClassNosyns(QStringLiteral("����"), checked);
        }   
    }
    else if ("����" == classname)
    {
        if (ui.checkBox_2->isChecked() != checked)
        {
            ui.checkBox_2->setChecked(checked);
            SetShowClassNosyns(QStringLiteral("����"), checked);
        }
    }
    else if ("����" == classname)
    {
        if (ui.checkBox_3->isChecked() != checked)
        {
            ui.checkBox_3->setChecked(checked);
            SetShowClassNosyns(QStringLiteral("����"), checked);
        }
    }
    else if ("������" == classname)
    {
        if (ui.checkBox_4->isChecked() != checked)
        {
            ui.checkBox_4->setChecked(checked);
            SetShowClassNosyns(QStringLiteral("������"), checked);
        }
    }
    else if ("����" == classname)
    {
        if (ui.checkBox_5->isChecked() != checked)
        {
            ui.checkBox_5->setChecked(checked);
            SetShowClassNosyns(QStringLiteral("����"), checked);
        }
    }
    else if ("��ײ��" == classname)
    {
        if (ui.checkBox_6->isChecked() != checked)
        {
            ui.checkBox_6->setChecked(checked);
            SetShowClassNosyns(QStringLiteral("��ײ��"), checked);
        } 
    }
}

void QPowerLineClassification::RemoveLineModels()
{
    vtkRenderer *render = ui.widget->viewer_->getRenderWindow()->GetRenderers()->GetFirstRenderer();
    for (auto it = line_models_.begin(); it!= line_models_.end();)
    {
        vtkProp * p = dynamic_cast<vtkProp *>(*it);
        if (p)
        {
            render->RemoveActor(p);
            ++it;
        }
        else
        {
            it = line_models_.erase(it);
        }
    }
    dd("�����%d", line_models_.size());
}

void QPowerLineClassification::SetShowClassNosyns(QString classname, bool checked/* = true*/)
{
    if (QStringLiteral("����") == classname)
    {
        ui.widget->viewer_->removePointCloud("ground");
        if (checked)
        {
            ui.widget->viewer_->addPointCloud(ground_, "ground");
        }
    }
    else if (QStringLiteral("����") == classname)
    {
        RemoveViewClouds("groundobject");
        if (checked)
        {
            for (int i = 0; i < groundObjects_.size(); ++i)
            {
                ui.widget->viewer_->addPointCloud(groundObjects_[i], (QStringLiteral("groundobject") + QString::number(i)).toLocal8Bit().data());
            }
        }
    }
    else if (QStringLiteral("����") == classname)
    {
        RemoveViewClouds("tower");
        if (checked)
        {
            for (int i = 0; i < towers_.size(); ++i)
            {
                ui.widget->viewer_->addPointCloud(towers_[i], (QStringLiteral("tower") + QString::number(i)).toLocal8Bit().data());
            }
        }
    }
    else if (QStringLiteral("������") == classname)
    {
        RemoveViewClouds("lines");
        if (checked)
        {
            for (int i = 0; i < lines_.size(); ++i)
            {
                ui.widget->viewer_->addPointCloud(lines_[i], (QStringLiteral("lines") + QString::number(i)).toLocal8Bit().data());
            }
        }
    }
    else if (QStringLiteral("������ģ��") == classname)
    {
        RemoveLineModels();
        if (checked)
        {
            for (int i = 0; i < lineInfos_.size(); ++i)
            {
                AddTube(lineInfos_[i]->fit_pts, 0.3);
            }
        }
    }
    else if (QStringLiteral("����") == classname)
    {
        ui.widget->viewer_->removePointCloud("cloud");
        if (checked)
        {
            ui.widget->viewer_->addPointCloud(cloud_, "cloud");
        }
    }
    else if (QStringLiteral("��ײ��") == classname)
    {
        RemoveViewBalls();
        if (checked)
        {
            for (int i = 0; i < balls_.size(); ++i)
            {
                ui.widget->viewer_->addSphere(balls_[i].cen, balls_[i].radiu, 1, 1, 0, balls_[i].id);
                ui.widget->viewer_->setShapeRenderingProperties(pcl::visualization::PCL_VISUALIZER_OPACITY, 0.2, balls_[i].id);
            }
        }
    }
}

void QPowerLineClassification::ShowSelectClass(bool checked)
{
    QCheckBox* box = qobject_cast<QCheckBox *>(sender());
    if (!box)
    {
        return;
    }
    QString text = box->text();

    
    SetShowClassNosyns(text, checked);


    UpdateView();
}

void QPowerLineClassification::UpdateView()
{
    //ui.widget->viewer_->resetCamera();
    ui.widget->update();
    QCoreApplication::processEvents();
}

void QPowerLineClassification::UpdateShowClass(std::string classname)
{
    if ("����" == classname)
    {
        {
            ui.checkBox->setChecked(true);
            SetShowClassNosyns(QStringLiteral("����"), true);
        }

    }
    else if ("����" == classname)
    {
        {
            ui.checkBox_2->setChecked(true);
            SetShowClassNosyns(QStringLiteral("����"), true);
        }
    }
    else if ("����" == classname)
    {
        {
            ui.checkBox_3->setChecked(true);
            SetShowClassNosyns(QStringLiteral("����"), true);
        }
    }
    else if ("������" == classname)
    {
        {
            ui.checkBox_4->setChecked(true);
            SetShowClassNosyns(QStringLiteral("������"), true);
        }
    }
    else if ("������ģ��" == classname)
    {
        {
            ui.checkBox_7->setChecked(true);
            SetShowClassNosyns(QStringLiteral("������ģ��"), true);
        }
    }
    else if ("����" == classname)
    {
        {
            ui.checkBox_5->setChecked(true);
            SetShowClassNosyns(QStringLiteral("����"), true);
        }
    }
    else if ("��ײ��" == classname)
    {
        {
            ui.checkBox_6->setChecked(true);
            SetShowClassNosyns(QStringLiteral("��ײ��"), true);
        }
    }
}

void QPowerLineClassification::Transitionlocal(PointCloudT &cloud, const vec &center)
{
    int cloudSize = cloud.size();
    for (int i = 0; i < cloudSize; ++i)
    {
        PointT &cur = cloud.at(i);
        cur.x -= center.x;
        cur.y -= center.y;
        cur.z -= center.z;
    }
}

void QPowerLineClassification::InitCloudInfo()
{
    PointT min, max;
    pcl::getMinMax3D(*cloud_, min, max);

    info_.count = cloud_->size();
    info_.setMinMax(min, max);
}

bool QPowerLineClassification::IsTower(PointCloudT::Ptr cloud, std::vector<uint> indices)
{
    float minX = std::numeric_limits<float>::max();
    float maxX = -std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = -std::numeric_limits<float>::max();
    for (int i = 0; i < indices.size(); ++i)
    {
        const float &curX = cloud->at((int)indices[i]).x;
        if (curX > maxX)
        {
            maxX = curX;
        }
        if (curX < minX)
        {
            minX = curX;
        }
        const float &curY = cloud->at((int)indices[i]).y;
        if (curY > maxY)
        {
            maxY = curY;
        }
        if (curY < minY)
        {
            minY = curY;
        }
        const float &curZ = cloud->at((int)indices[i]).z;
        if (curZ > maxZ)
        {
            maxZ = curZ;
        }
        if (curZ < minZ)
        {
            minZ = curZ;
        }
    }
    
    float subx = maxX - minX;
    float suby = maxY - minY;
    float subz = maxZ - minZ;
    if (subz > subx && subz > suby && subz > 10)
        return true;
    else
        return false;
}
#include <vtkOBJExporter.h>  
void QPowerLineClassification::ExportCurView()
{
    vtkSmartPointer<vtkOBJExporter> porter = vtkSmartPointer<vtkOBJExporter>::New();
    porter->SetFilePrefix("E:\\qiu.obj");
    porter->SetInput(ui.widget->viewer_->getRenderWindow());
    porter->Update();
}
