#pragma once

#include <QSubDialogBase.h>
#include "ui_QPowerLineClassification.h"
#include <PCManage.h>
#include <pcl/kdtree/kdtree_flann.h>
#include "LeastSquare.h"
#include "MathGeoLibFwd.h"
#include "Math/float3.h"

double GetDistance3d(PointT &pt1, PointT &pt2);
double GetDistance2d(PointT &pt1, PointT &pt2);
double GetDistance2d(double x1, double y1, double x2, double y2);
double GetDistance2d(const vec &pt1, const vec &pt2);
typedef struct
{
    enum Asix{X=0, Y=1, Z=2};
    vec sta;
    vec end;
    vec v;
    Asix maxAsix;
    std::vector<vec> pts;
    std::vector<vec> fit_pts;
    Fit fit;
    Fit fit_z;
} LineInfo;
typedef struct
{
    PointT cen;
    double radiu;
    std::string id;
    std::string description;
}CollisionBall;

struct CloudInfo
{
    vec min, max, center;
    int count;

    CloudInfo() :count(0){};
    void setMinMax(const PointT &omin, PointT &omax)
    {
        min.x = omin.x;
        min.y = omin.y;
        min.z = omin.z;
        max.x = omax.x;
        max.y = omax.y;
        max.z = omax.z;

        center = (min + max) / 2;
    }
};
vec PointT2Vec(const PointT &ptt);
PointT Vec2PointT(const vec &ptv);

class QPowerLineClassification : public QSubDialogBase
{
	Q_OBJECT

public:
	QPowerLineClassification(QWidget *parent = Q_NULLPTR);
	~QPowerLineClassification();
	

	void InitData();//��ʼ������
    void SaveSetting();//��������

private:
	PointCloudT::Ptr cloud_;//ά�������ĵ�������
    CloudInfo info_;                //��ǰ������Ϣ

    PointCloudT::Ptr ground_;//�����
    std::vector<PointCloudT::Ptr> groundObjects_;//��������
    std::vector<PointCloudT::Ptr> towers_;//��������
    std::vector<PointCloudT::Ptr> lines_;//��������
    std::vector<boost::shared_ptr<LineInfo>> lineInfos_;//��������Ϣ���������յ������
    std::vector<CollisionBall> balls_;//��ײ���α�ע
    std::set<vtkProp *> line_models_;
    
    
	Ui::QPowerLineClassification ui;//����

    // ransac������ȡԲ��
	//void RanSacArc();

    void InitCloudInfo();

	// �Ƿ���ϵ����ߵ��ص�
    bool LikePowerLine(LineInfo &line);

    // �Ƿ���������ص�
    bool LikeTower(PointCloudT &cloud);

    // ��鷨��
	bool CheckNormal(PointCloudT &cloud);

    // ��xyƽ���Ƿ���ֱ��
	bool IsXyPlaneLine(PointCloudT &cloud);

    // ɾ�����ܶ���
	void RemoveHighDensityClass();

    // ɾ��aabb��Χ���쳣��
	void RemoveAABBUnusual();

    // �����ƴ���xyƽ���Ӧ�Ķ�ά����
    void PutPointCloud2Arr(PointCloudT::Ptr cloud, std::vector<std::vector<pcl::PointIndices::Ptr>> &pointsArr, int row, int col, PointT min, PointT max, double gridSize);

    // �����ƴ���xyƽ���Ӧ�Ķ�ά����
    void PutPointCloud2Arr(PointCloudT::Ptr cloud, std::vector<std::vector<pcl::PointIndices::Ptr>> &pointsArr, PointCloudT::Ptr ground, std::vector<std::vector<pcl::PointIndices::Ptr>> &groundArr, int row, int col, PointT min, PointT max, double gridSize);
    
    // �Ƿ���ܶȹ���
	bool IsHighDensity(PointCloudT &cloud, float density, float leafSize, float neighborNum);

    // Ӧ�þ����ĵ���
	void UpdateJLClusters2Cloud();

    // aabb��Χ���Ƿ��쳣
	bool IsAABBUnusual(PointCloudT &cloud, double aabbLimit);

    //obb��Χ���Ƿ��쳣
    bool IsOBBUnusual(PointCloudT &cloud, double obbLimit = 100);

    // �������Ƿ������Ⱥ��
    bool IsHaveLiQunDian(PointCloudT::Ptr cloud);

    // ��ȡ��������
	void RemoveLowerClass();

    // ���ɹ���
    void AddTube(const std::vector<vec> &pts, const float raidu);
    void AddTube(const boost::shared_ptr<LineInfo> &line, const float radiu);

    // ���ɶ����
    void AddVtkLines(const boost::shared_ptr<LineInfo> &line);

    // �Ƿ����������
	bool IsLowerClass(PointCloudT &cloud, pcl::KdTreeFLANN<PointT> &ground_kdtree, double groundLower, double groundLnterval, double aabbHeight);

    // �Ƿ����������
    bool IsLowerClass(PointCloudT &cloud, pcl::KdTreeFLANN<PointT> &ground_kdtree, double groundLower);
    // ��ȡobb��Χ��
	vec GetObbBox(PointCloudT &cloud);

    // �ռ䷶Χ����
    void DistanceSerach(double cenX, double cenY, double cenZ, double dis, std::vector<int> &indices);

    // �ռ���z�����
	void SerachVecRangePoints(PointT pt, double dis, pcl::PointIndices::Ptr vecPoints, pcl::PointIndices::Ptr vecrangePoint);

    // �ռ���z�����
    void SerachVecRangePoints(PointT pt, double dis, double minz, double maxz, pcl::PointIndices::Ptr vecrangePoint);

    // ȥ���ظ���
    void NoRepeat(pcl::PointIndices::Ptr vecrangePoint);

    // ȥ���ظ���
    void NoRepeat(std::vector<uint>& indices);

    // ��ʾ���������
    void ShowTooNearCheck();

    // ��ע���������
    void LabelCrashLine(PointCloudT::Ptr crashPoint, double K, double gap);

    // ��ʾ�����߽��
    void ShowTowerLines();

    // �������лָ�
    void RecoverGroundObject();

    // ��ȡ�����ߵ������˵�
    void CalcLineInfo(PointCloudT::Ptr cloud, LineInfo &info);

    // �����ߺϲ�
    void LineConnect();

    // �������Ƿ�ӽ�
    bool IsConnectLine(const LineInfo &l1, const LineInfo &l2);

    // �����Ƿ�һ��
    bool IsSimilarityVec(vec &v1, vec &v2, double error = 0.15);

    // �����Ƿ�һ��
    bool IsSimilarityVec(vec &sta, vec &end, vec& sumvec);

    // ��ȡ��������������ά�ȵĴ��С
    vec GetMaxAxisVec(vec &vpt1, vec& vpt2, bool bSwap = false);

    // ��ȡ��������������ά�ȵĴ��С
    vec GetMaxAxisVec(const LineInfo &info);

    // ��ȡ�����0x,1y
    int GetXyMaxAsix(vec &sta, vec& end);

    // ����������
    void swapvec(vec &v1, vec &v2);

    // �ж��Ƿ�������
    bool IsTower(PointCloudT::Ptr cloud, std::vector<uint> indices);

    // ��������z���
    float GetClustersHeight(const std::vector<uint> &clusters, const PointCloudT &cloud_);

    // �������лָ�
    void RecoverGround(PointCloudT::Ptr ground, PointCloudT::Ptr outCloud);

    // ��ɾ��view���ָ������
    void RemoveViewClouds(QString name);

    // ��view�����ָ������
    void SetShowClassNosyns(QString classname, bool show = true);

    // ��view�����ָ������
    void SetShowClass(std::string classname, bool checked = true);

    // ��view�����ָ������
    void UpdateShowClass(std::string classname);

    // ��view��ɾ��������ײ��
    void RemoveViewBalls();

    // ��view��ɾ��������ģ��
    void RemoveLineModels();

    // ˢ��view
    void UpdateView();

    // �����ʾ����
    void ClearShowClass();

    // ������˸߳�ֱ��ͼ��
    void ExtractGroundOnElevationHistogram();

    public slots:
    // һ��ʶ��ť
	void OnApply();

    // ��������չ����������ҳ��
	void OnGroupBoxCheck(bool cheched);
    
    // ������ȡ
    void GroundObjectFilter();

    // ������ȡ
	void ExtractTower();

    // ��������ȡ
    void ExtractPowerLine();

    // ������������ͼ
	void UpdateMainView();

    // ��ײ���
    void TooNearCheck();
    void TooNearCheck2();

    // �������ȡ
    void RoughExtractGroundObject();

    // ��������normal������color����
    void QuyuzengzhangNormalAndColor();

    // ָ����ʾ������
    void ShowSelectClass(bool checked);

    // ��ȡ����
    void ExtractGround();

    // ������ǰ��ͼ
    void ExportCurView();

    void Simplify();

    // ʹ��opencvѰ��ֱ�� 
    void UseOpencvFineLine();

};
