/**
This code comes from this paper:

Hu, Ruizhen, et al. "Interaction Context (ICON): Towards a Geometric Functionality Descriptor." ACM Transactions on Graphics 34.
*/

#include "Sampler.h"
#include "../Utilities/Utilities.h"

Sampler::Sampler(const Mesh &srcMesh, SamplingMethod samplingMethod)
{
    /* srand (123456789); */
    /*if(srcMesh == NULL) 
        return;
    else*/
    mesh = srcMesh.mesh3d;
    method = samplingMethod;
    
    // mesh->update_face_normals(); // OK, as my normals are all calculated in the beginning
    bool exists;
    boost::tie(fnormal,exists) = mesh->property_map<Face,Vector3d>("f:normal");

    //SurfaceMeshHelper h(mesh);
    boost::tie(farea,exists) = mesh->property_map<Face,double>("f:area");
    points = mesh->points();

    // FaceBarycenterHelper fh(mesh);
    boost::tie(fcenter,exists) = mesh->property_map<Face,Point3d>("f:center");
    boost::tie(fpt,exists) = mesh->property_map<Face,Point3d>("f:pt");

    // Sample based on method selected
    if( method == RANDOM_BARYCENTRIC_AREA || RANDOM_BARYCENTRIC_WEIGHTED || RANDOM_BARYCENTRIC_WEIGHTED_DISTANCE)
    {
        bool created;
        FaceProperty<double> fprobability;
        boost::tie(fprobability, created) = mesh->add_property_map<Face,double>("f:probability");
        if (method == RANDOM_BARYCENTRIC_AREA)
        {
            // Compute all faces area
            // fprobability = mesh->face_property<Scalar>("f:probability", 0);

            double totalMeshArea = 0;
            // Surface_mesh::Face_iterator fit, fend = mesh->faces_end();

            BOOST_FOREACH(Face f_id, mesh->faces())
                totalMeshArea += farea[f_id];

            BOOST_FOREACH(Face f_id, mesh->faces())
                fprobability[f_id] = farea[f_id] / totalMeshArea;
        }
        else if( method == RANDOM_BARYCENTRIC_WEIGHTED_DISTANCE)
        {
            // fprobability = mesh->face_property<Scalar>("f:probability", 0);
            FaceProperty<double> fweight;
            boost::tie(fweight, created) = mesh->property_map<Face,double>("f:weight");
            // ScalarFaceProperty fweight = mesh->get_face_property<Scalar>("f:weight");
            double totalWeight = 0;
            double totalMeshArea = 0;
            // Surface_mesh::Face_iterator fit, fend = mesh->faces_end();
            BOOST_FOREACH(Face f_id, mesh->faces())
            {
                totalWeight += fweight[f_id];
                totalMeshArea += farea[f_id];
            }

            BOOST_FOREACH(Face f_id, mesh->faces()) 
            {
              fprobability[f_id] = 0.5*(fweight[f_id] / totalWeight)+0.5*(farea[f_id] / totalMeshArea);
              //std::vector<Vertex> fv = srcMesh.getVerticesOfFace(f_id);
              //DebugLogger::ss << "Face (" << srcMesh.mesh3d->point(fv[0]) << "," << srcMesh.mesh3d->point(fv[1]) << "," << srcMesh.mesh3d->point(fv[2]) << ") prob:" << fprobability[f_id];
              //DebugLogger::log();
            }
        }
        else
        {
            // fprobability = mesh->face_property<Scalar>("f:probability", 0);
            FaceProperty<double> fweight;
            boost::tie(fweight, created) = mesh->property_map<Face,double>("f:weight");
            // ScalarFaceProperty fweight = mesh->get_face_property<Scalar>("f:weight");
            double totalWeight = 0;
            // Surface_mesh::Face_iterator fit, fend = mesh->faces_end();
            BOOST_FOREACH(Face f_id, mesh->faces())
                totalWeight += fweight[f_id];

            BOOST_FOREACH(Face f_id, mesh->faces()) 
            {
              fprobability[f_id] = fweight[f_id] / totalWeight;
              //std::vector<Vertex> fv = srcMesh.getVerticesOfFace(f_id);
              //DebugLogger::ss << "Face (" << srcMesh.mesh3d->point(fv[0]) << "," << srcMesh.mesh3d->point(fv[1]) << "," << srcMesh.mesh3d->point(fv[2]) << ") prob:" << fprobability[f_id];
              //DebugLogger::log();
            }
        }

        interval = std::vector<WeightFace>(mesh->number_of_faces() + 1);
        interval[0] = WeightFace(0.0, Face(0));
        int i = 0;

        // Compute mesh area in a cumulative manner
        BOOST_FOREACH(Face f_id, mesh->faces())
        {
            interval[f_id+1] = WeightFace(interval[i].weight + fprobability[f_id], f_id);
            i++;
        }
    }
    else if( method ==  FACE_CENTER_RANDOM || method == FACE_CENTER_ALL )
    {
        // No preparations needed..
    }
}

SamplePoint Sampler::getSample(double weight)
{
    SamplePoint sp;
    double r;
    double b[3];

    if( method == RANDOM_BARYCENTRIC_AREA || RANDOM_BARYCENTRIC_WEIGHTED || RANDOM_BARYCENTRIC_WEIGHTED_DISTANCE)
    {
        // r, random point in the area
        /* r = uniform(); */
        r = utilities::uniformDouble(0,1);

        // Find corresponding face
        std::vector<WeightFace>::iterator it = lower_bound(interval.begin(), interval.end(), WeightFace(std::min(r,interval.back().weight)));
        Face f = it->f;

        /* if(method == RANDOM_BARYCENTRIC_WEIGHTED_DISTANCE) { */
        /*   std::vector<Point3d> v; */
        /*   CGAL::Vertex_around_face_iterator<Mesh3d> vbegin, vend; */
        /*   for(boost::tie(vbegin, vend) = CGAL::vertices_around_face(mesh->halfedge(f), *mesh); */
        /*       vbegin != vend; */ 
        /*       ++vbegin){ */
        /*       v.push_back(mesh->point(*vbegin)); */
        /*   } */
        /*   Eigen::VectorXd v0(3), v1(3), v2(3), p(3); */
        /*   v0 << v[0].x(), v[0].y(), v[0].z(); */
        /*   v1 << v[1].x(), v[1].y(), v[1].z(); */
        /*   v2 << v[2].x(), v[2].y(), v[2].z(); */
        /*   p << fpt[f].x(), fpt[f].y(), fpt[f].z(); */
        /*   Point3d pos = weightedBaricentric(p, v0, v1, v2); */

        /*   sp = SamplePoint( pos, fnormal[f], weight, f); */

        /* } else { */
          // Add sample from that face
          RandomBaricentric(b);

          sp = SamplePoint( getBaryFace(f, b[0], b[1]), fnormal[f], weight, f, b[0], b[1]);
        /* } */
    }
    else if( method ==  FACE_CENTER_RANDOM )
    {
        int fcount = mesh->number_of_faces();

        int randTriIndex = (int) (fcount * (((double)rand()) / (double)RAND_MAX)) ;

        if( randTriIndex >= fcount )
            randTriIndex = fcount - 1;

        Face f(randTriIndex);

        // Get triangle center and normal
        sp = SamplePoint(fcenter[f], fnormal[f], farea[f], f, 1 / 3.0, 1 / 3.0);
    }

    return sp;
}

std::vector<SamplePoint> Sampler::getSamples(int numberSamples, double weight)
{
    std::vector<SamplePoint> samples;
    DebugLogger::ss << "Getting samples ...";
    DebugLogger::log();

    if (method == FACE_CENTER_ALL)
    {
        BOOST_FOREACH(Face f_id, mesh->faces())
            samples.push_back(SamplePoint(fcenter[f_id], fnormal[f_id], 0, f_id, 1 / 3.0, 1 / 3.0));
    }
    else
    {
        for(int i = 0; i < numberSamples; i++)
        {
            samples.push_back(getSample(weight));
        }
    }
    return samples;
}

Point3d Sampler::getBaryFace( Face f_id, double U, double V )
{
    // vector<Vector3d> v;
    // Surface_mesh::Vertex_around_face_circulator vit = mesh->vertices(f),vend=vit;
    // do{ v.push_back(points[vit]); } while(++vit != vend);
    std::vector<Point3d> v;
    CGAL::Vertex_around_face_iterator<Mesh3d> vbegin, vend;
    for(boost::tie(vbegin, vend) = CGAL::vertices_around_face(mesh->halfedge(f_id), *mesh);
        vbegin != vend; 
        ++vbegin){
        v.push_back(mesh->point(*vbegin));
    }

    if(U == 1.0) return v[1];
    if(V == 1.0) return v[2];

    double b1 = U;
    double b2 = V;
    double b3 = 1.0 - (U + V);

    double x = (b1 * v[0].x()) + (b2 * v[1].x()) + (b3 * v[2].x());
    double y = (b1 * v[0].y()) + (b2 * v[1].y()) + (b3 * v[2].y());
    double z = (b1 * v[0].z()) + (b2 * v[1].z()) + (b3 * v[2].z());
    return Point3d(x,y,z);
}

