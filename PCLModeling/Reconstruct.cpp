#include "Reconstruct.h"
#include <QRendView.h>
#include "dvprint.h"

//#define UseCGAL
#define UsePCL
// #define UseLVR
// #define UseMESHLAB

void Reconstruct::BuildSceneMesh(PointCloudT::Ptr cloud)
{
#if defined UseCGAL
    CGALReconstruct(cloud);
#elif defined UsePCL
    // PCLPoissonReconstruct(cloud); //�ɰ��ؽ�
    // PclCubeReconstruct(cloud); //�ƶ��������ؽ�
    PclGp3Reconstruct(cloud); //̰�����ǽ�ģ
#elif defined UseLVR
    LVReconstruct(cloud);
#elif defined UseMESHLAB
    MESHLABeconstruct(cloud);
#endif //

}

#ifdef UseCGAL
#include <CGAL/trace.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/IO/Polyhedron_iostream.h>
#include <CGAL/Surface_mesh_default_triangulation_3.h>
#include <CGAL/make_surface_mesh.h>
#include <CGAL/Implicit_surface_3.h>
#include <CGAL/IO/facets_in_complex_2_to_triangle_mesh.h>
#include <CGAL/Poisson_reconstruction_function.h>
#include <CGAL/Point_with_normal_3.h>
#include <CGAL/property_map.h>
#include <CGAL/Poisson_reconstruction_function.h>
#include <CGAL/IO/read_xyz_points.h>
#include <CGAL/compute_average_spacing.h>
#include <CGAL/Polygon_mesh_processing/distance.h>
#include <CGAL/pca_estimate_normals.h>
#include <vector>
#include <CGAL/IO/STL_writer.h>
#include <fstream>
#include <CGAL\Eigen_solver_traits.h>
#include <Eigen\src\IterativeLinearSolvers\ConjugateGradient.h>
#include <CGAL\Eigen_matrix.h>
#include <iostream>
#include <pcl/PolygonMesh.h>
#include <ostream>
// Types
typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef Kernel::FT FT;
typedef Kernel::Point_3 Point;
typedef CGAL::Point_with_normal_3<Kernel> Point_with_normal;
typedef Kernel::Sphere_3 Sphere;
typedef std::vector<Point_with_normal> PointList;
typedef CGAL::Polyhedron_3<Kernel> Polyhedron;
typedef CGAL::Poisson_reconstruction_function<Kernel> Poisson_reconstruction_function;
typedef CGAL::Surface_mesh_default_triangulation_3 STr;
typedef CGAL::Surface_mesh_complex_2_in_triangulation_3<STr> C2t3;
typedef CGAL::Implicit_surface_3<Kernel, Poisson_reconstruction_function> Surface_3;
typedef Kernel::Vector_3 Vector;

// Point with normal vector stored in a std::pair.
// typedef std::pair<Point, Vector> PointVectorPair;
// typedef std::vector<PointVectorPair> PointList;

// Concurrency
#ifdef CGAL_LINKED_WITH_TBB
typedef CGAL::Parallel_tag Concurrency_tag;
#else
typedef CGAL::Sequential_tag Concurrency_tag;
#endif

void mesh2stl(Polyhedron& P)
{
    const char* oname = "out";
    boost::shared_ptr<std::ofstream> p_out(new std::ofstream(oname));
    *p_out << "solid " << oname << std::endl;
    auto vi = P.vertices_begin();
    Point p = vi->point();
    double minx = p.x();
    double miny = p.y();
    double minz = p.z();
    for (; vi != P.vertices_end(); ++vi) {
        p = vi->point();
        if (p.x() < minx)
            minx = p.x();
        if (p.y() < miny)
            miny = p.y();
        if (p.z() < minz)
            minz = p.z();
    }
    // translate into positive octant
    Vector trans(-minx, -miny, -minz);
    for (auto i = P.vertices_begin(); i != P.vertices_end(); ++i) {
        i->point() = i->point() + trans;
    }
    // write triangles
    for (auto i = P.facets_begin(); i != P.facets_end(); ++i) {
        auto h = i->halfedge();
        if (h->next()->next()->next() != h) {
            return;
        }
        Point p = h->vertex()->point();
        Point q = h->next()->vertex()->point();
        Point r = h->next()->next()->vertex()->point();
        // compute normal
        Vector n = CGAL::cross_product(q - p, r - p);
        Vector norm = n / std::sqrt(n * n);
        *p_out << "    facet normal " << norm << std::endl;
        *p_out << "      outer loop " << std::endl;
        *p_out << "        vertex " << p << std::endl;
        *p_out << "        vertex " << q << std::endl;
        *p_out << "        vertex " << r << std::endl;
        *p_out << "      endloop " << std::endl;
        *p_out << "    endfacet " << std::endl;
    }

    *p_out << "endsolid " << oname << std::endl;
    p_out->flush();
    p_out->close();
}
void Reconstruct::CGALReconstruct(PointCloudT::Ptr cloud)
{
    unsigned int nb_neighbors_pca_normals = 16;
    // Poisson options
    FT sm_angle = 20.0; // Min triangle angle in degrees.
    FT sm_radius = 30; // Max triangle size w.r.t. point set average spacing.
    FT sm_distance = 0.5; // Surface Approximation error w.r.t. point set average spacing.
    // Reads the point set file in points[].
    // Note: read_xyz_points_and_normals() requires an iterator over points
    // + property maps to access each point's position and normal.
    // The position property map can be omitted here as we use iterators over Point_3 elements.
    PointList points;
    for (int i = 0; i < cloud->size(); ++i)
    {
        PointT &pt = cloud->at(i);
        Point_with_normal cgpt;
        Vector v3(pt.x, pt.y, pt.z);
        cgpt += v3;
        points.push_back(cgpt);
    }
    CGAL::pca_estimate_normals<Concurrency_tag>(points,
        nb_neighbors_pca_normals,
        CGAL::parameters::normal_map(CGAL::make_normal_of_point_with_normal_map(PointList::value_type())));


    //     std::ifstream stream("data/kitten.xyz");
    //     if (!stream ||
    //         !CGAL::read_xyz_points(
    //         stream,
    //         std::back_inserter(points),
    //         CGAL::parameters::normal_map(CGAL::make_normal_of_point_with_normal_map(PointList::value_type()))))
    //     {
    //         std::cerr << "Error: cannot read file data/kitten.xyz" << std::endl;
    //         return ;
    //     }

    // Creates implicit function from the read points using the default solver.
    // Note: this method requires an iterator over points
    // + property maps to access each point's position and normal.
    // The position property map can be omitted here as we use iterators over Point_3 elements.
    Poisson_reconstruction_function function(points.begin(), points.end(),
        CGAL::make_normal_of_point_with_normal_map(PointList::value_type()));
    // Computes the Poisson indicator function f()
    // at each vertex of the triangulation.
    if (!function.compute_implicit_function())
        return;
    // Computes average spacing
    FT average_spacing = CGAL::compute_average_spacing<CGAL::Sequential_tag>(points, 16 /* knn = 1 ring */);
    // Gets one point inside the implicit surface
    // and computes implicit function bounding sphere radius.
    Point inner_point = function.get_inner_point();
    Sphere bsphere = function.bounding_sphere();
    FT radius = std::sqrt(bsphere.squared_radius());
    // Defines the implicit surface: requires defining a
    // conservative bounding sphere centered at inner point.
    FT sm_sphere_radius = 5.0 * radius;
    
    
    FT sm_dichotomy_error = sm_distance*average_spacing / 1000.0; // Dichotomy error must be << sm_distance
    Surface_3 surface(function,
        Sphere(inner_point, sm_sphere_radius*sm_sphere_radius),
        sm_dichotomy_error / sm_sphere_radius);
    dd("2");
    // Defines surface mesh generation criteria
    CGAL::Surface_mesh_default_criteria_3<STr> criteria(sm_angle,  // Min triangle angle (degrees)
        sm_radius*average_spacing,  // Max triangle size
        sm_distance*average_spacing); // Approximation error
    // Generates surface mesh with manifold option
    STr tr; // 3D Delaunay triangulation for surface mesh generation
    C2t3 c2t3(tr); // 2D complex in 3D Delaunay triangulation
    CGAL::make_surface_mesh(c2t3,                                 // reconstructed mesh
        surface,                              // implicit surface
        criteria,                             // meshing criteria
        CGAL::Manifold_with_boundary_tag());  // require manifold mesh
    dd("3");
    if (tr.number_of_vertices() == 0)
        return;
    // saves reconstructed surface mesh
    std::ofstream out("kitten_poisson-20-30-0.375.off");
    CGAL::set_binary_mode(out);
    Polyhedron output_mesh;
    dd("4");
    CGAL::facets_in_complex_2_to_triangle_mesh(c2t3, output_mesh);
    dd("5");
    CGAL::write_STL<Polyhedron>(output_mesh, out);
    //mesh2stl(output_mesh);
    dd("6");
    out << output_mesh;
    // computes the approximation error of the reconstruction
    double max_dist =
        CGAL::Polygon_mesh_processing::approximate_max_distance_to_point_set(output_mesh,
        points,
        4000);
    std::cout << "Max distance to point_set: " << max_dist << std::endl;
}
#endif //UseCGAL


#ifdef UsePCL
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
void Reconstruct::PCLPoissonReconstruct(PointCloudT::Ptr color_cloud)
{
    if (!color_cloud)
    {
        return;
    }
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>());
    cloud->resize(color_cloud->size());
    for (int i = 0; i < cloud->size(); ++i)
    {
        cloud->at(i).x = color_cloud->at(i).x;
        cloud->at(i).y = color_cloud->at(i).y;
        cloud->at(i).z = color_cloud->at(i).z;
    }
    // ���㷨����
    pcl::PointCloud<pcl::PointNormal>::Ptr cloud_with_normals(new pcl::PointCloud<pcl::PointNormal>); //���������ƶ���ָ��
    pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> n;//���߹��ƶ���
    pcl::PointCloud<pcl::Normal>::Ptr normals(new pcl::PointCloud<pcl::Normal>);//�洢���Ƶķ��ߵ�ָ��
    pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);
    tree->setInputCloud(cloud);
    n.setInputCloud(cloud);
    n.setSearchMethod(tree);
    n.setKSearch(16);
    n.compute(*normals); //���㷨�ߣ�����洢��normals��

    //�����ƺͷ��߷ŵ�һ��
    pcl::concatenateFields(*cloud, *normals, *cloud_with_normals);

    //����������
    pcl::search::KdTree<pcl::PointNormal>::Ptr tree2(new pcl::search::KdTree<pcl::PointNormal>);
    tree2->setInputCloud(cloud_with_normals);
    //����Poisson���󣬲����ò���
    pcl::Poisson<pcl::PointNormal> pn;
    pn.setConfidence(false); //�Ƿ�ʹ�÷������Ĵ�С��Ϊ������Ϣ�����false�����з���������һ����
    pn.setDegree(2); //���ò���degree[1,5],ֵԽ��Խ��ϸ����ʱԽ�á�
    pn.setDepth(8); //���������ȣ����2^d x 2^d x 2^d������Ԫ�����ڰ˲�������Ӧ�����ܶȣ�ָ��ֵ��Ϊ�����ȡ�
    pn.setIsoDivide(8); //������ȡISO��ֵ����㷨�����
    pn.setManifold(false); //�Ƿ���Ӷ���ε����ģ�����������ǻ�ʱ�� �������б�־���������Ϊtrue����Զ���ν���ϸ�����ǻ�ʱ������ģ�����false�����
    pn.setOutputPolygons(false); //�Ƿ������������񣨶��������ǻ��ƶ�������Ľ����
    pn.setSamplesPerNode(3.0); //��������һ���˲�������е����������С��������������[1.0-5.0],������[15.-20.]ƽ��
    pn.setScale(1.25); //���������ع���������ֱ���������߽�������ֱ���ı��ʡ�
    pn.setSolverDivide(8); //����������Է������Gauss-Seidel�������������
    //pn.setIndices();

    //���������������������
    pn.setSearchMethod(tree2);
    pn.setInputCloud(cloud_with_normals);
    //����������������ڴ洢���
    pcl::PolygonMesh mesh;
    //ִ���ع�
    pn.performReconstruction(mesh);

    //��������ͼ
    pcl::io::savePLYFile("result.ply", mesh);

    // ��ʾ���ͼ
    boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer(new pcl::visualization::PCLVisualizer("PCLPoissonReconstruct"));
    viewer->setBackgroundColor(0.5, 0.5, 0.5);
    viewer->addPolygonMesh(mesh, "my");
    viewer->addCoordinateSystem(50.0);
    viewer->initCameraParameters();
    while (!viewer->wasStopped()){
        viewer->spinOnce(100);
        boost::this_thread::sleep(boost::posix_time::microseconds(100000));
    }
}

void Reconstruct::PclCubeReconstruct(PointCloudT::Ptr color_cloud)
{
    if (!color_cloud)
    {
        return;
    }
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>());
    cloud->resize(color_cloud->size());
    for (int i = 0; i < cloud->size(); ++i)
    {
        cloud->at(i).x = color_cloud->at(i).x;
        cloud->at(i).y = color_cloud->at(i).y;
        cloud->at(i).z = color_cloud->at(i).z;
    }
    // ���Ʒ�����
    pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> n;
    pcl::PointCloud<pcl::Normal>::Ptr normals(new pcl::PointCloud<pcl::Normal>);
    pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);
    tree->setInputCloud(cloud);
    n.setInputCloud(cloud);
    n.setSearchMethod(tree);
    n.setKSearch(16);
    n.compute(*normals); //���㷨�ߣ�����洢��normals��
    //* normals ����ͬʱ������ķ������ͱ��������

    //�����ƺͷ��߷ŵ�һ��
    pcl::PointCloud<pcl::PointNormal>::Ptr cloud_with_normals(new pcl::PointCloud<pcl::PointNormal>);
    pcl::concatenateFields(*cloud, *normals, *cloud_with_normals);
    //* cloud_with_normals = cloud + normals


    //����������
    pcl::search::KdTree<pcl::PointNormal>::Ptr tree2(new pcl::search::KdTree<pcl::PointNormal>);
    tree2->setInputCloud(cloud_with_normals);

    //��ʼ��MarchingCubes���󣬲����ò���
    pcl::MarchingCubes<pcl::PointNormal> *mc;
    mc = new pcl::MarchingCubesHoppe<pcl::PointNormal>();
    /*
    if (hoppe_or_rbf == 0)
    mc = new pcl::MarchingCubesHoppe<pcl::PointNormal> ();
    else
    {
    mc = new pcl::MarchingCubesRBF<pcl::PointNormal> ();
    (reinterpret_cast<pcl::MarchingCubesRBF<pcl::PointNormal>*> (mc))->setOffSurfaceDisplacement (off_surface_displacement);
    }
    */

    //����������������ڴ洢���
    pcl::PolygonMesh mesh;

    //����MarchingCubes����Ĳ���
    mc->setIsoLevel(0.0f);
    mc->setGridResolution(100, 100, 100);
    mc->setPercentageExtendGrid(0.0f);

    //������������
    mc->setInputCloud(cloud_with_normals);

    //ִ���ع������������mesh��
    mc->reconstruct(mesh);

    //��������ͼ
    pcl::io::savePLYFile("result.ply", mesh);

    // ��ʾ���ͼ
    boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer(new pcl::visualization::PCLVisualizer("cube"));
    viewer->setBackgroundColor(0.5, 0.5, 0.5); //���ñ���
    viewer->addPolygonMesh(mesh, "my"); //������ʾ������
    viewer->addCoordinateSystem(1.0); //��������ϵ
    viewer->initCameraParameters();
    while (!viewer->wasStopped()){
        viewer->spinOnce(100);
        boost::this_thread::sleep(boost::posix_time::microseconds(100000));
    }

}

#include <pcl/surface/gp3.h>
void Reconstruct::PclGp3Reconstruct(PointCloudT::Ptr color_cloud)
{
    if (!color_cloud)
    {
        return;
    }
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>());
    cloud->resize(color_cloud->size());
    for (int i = 0; i < cloud->size(); ++i)
    {
        cloud->at(i).x = color_cloud->at(i).x;
        cloud->at(i).y = color_cloud->at(i).y;
        cloud->at(i).z = color_cloud->at(i).z;
    }


    // Normal estimation*
    pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> n;
    pcl::PointCloud<pcl::Normal>::Ptr normals(new pcl::PointCloud<pcl::Normal>);
    pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);
    tree->setInputCloud(cloud);
    n.setInputCloud(cloud);
    n.setSearchMethod(tree);
    n.setKSearch(16);
    n.compute(*normals);
    //* normals should not contain the point normals + surface curvatures

    // Concatenate the XYZ and normal fields*
    pcl::PointCloud<pcl::PointNormal>::Ptr cloud_with_normals(new pcl::PointCloud<pcl::PointNormal>);
    pcl::concatenateFields(*cloud, *normals, *cloud_with_normals);
    //* cloud_with_normals = cloud + normals

    // Create search tree*
    pcl::search::KdTree<pcl::PointNormal>::Ptr tree2(new pcl::search::KdTree<pcl::PointNormal>);
    tree2->setInputCloud(cloud_with_normals);

    // Initialize objects
    pcl::GreedyProjectionTriangulation<pcl::PointNormal> gp3;
    pcl::PolygonMesh triangles;

    // Set the maximum distance between connected points (maximum edge length)
    gp3.setSearchRadius(3);

    // Set typical values for the parameters
    gp3.setMu(2.5);
    gp3.setMaximumNearestNeighbors(100);
    gp3.setMaximumSurfaceAngle(M_PI / 4); // 45 degrees
    gp3.setMinimumAngle(M_PI / 18); // 10 degrees
    gp3.setMaximumAngle(2 * M_PI / 3); // 120 degrees
    gp3.setNormalConsistency(false);

    // Get result
    gp3.setInputCloud(cloud_with_normals);
    gp3.setSearchMethod(tree2);
    gp3.reconstruct(triangles);

    // Additional vertex information
    std::vector<int> parts = gp3.getPartIDs();
    std::vector<int> states = gp3.getPointStates();

    boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer(new pcl::visualization::PCLVisualizer("triangles"));
    viewer->setBackgroundColor(0.5, 0.5, 0.5);

    viewer->addPolygonMesh(triangles, "triangles");

    viewer->addCoordinateSystem(1.0);
    viewer->initCameraParameters();
    while (!viewer->wasStopped()){
        viewer->spinOnce(100);
        boost::this_thread::sleep(boost::posix_time::microseconds(100000));
    }
}

#endif