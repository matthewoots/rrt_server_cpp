/*
 * rrt_search_module.h
 *
 * ---------------------------------------------------------------------
 * Copyright (C) 2022 Matthew (matthewoots at gmail.com)
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ---------------------------------------------------------------------
 *
 * 
 * 
 */

/*
* With help from 
* https://github.com/swadhagupta/RRT/blob/master/rrt.cpp
* https://pcl.readthedocs.io/projects/tutorials/en/latest/kdtree_search.html
*/
#ifndef RRT_SEARCH_MODULE_H
#define RRT_SEARCH_MODULE_H

#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include <algorithm>
#include <limits>
#include <random>
#include <chrono>
#include <Eigen/Dense>

#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/filters/crop_box.h>

using namespace Eigen;
using namespace std;
using namespace std::chrono;

#define dmax std::numeric_limits<double>::max();

#define KNRM  "\033[0m"
#define KRED  "\033[31m"
#define KGRN  "\033[32m"
#define KYEL  "\033[33m"
#define KBLU  "\033[34m"
#define KMAG  "\033[35m"
#define KCYN  "\033[36m"
#define KWHT  "\033[37m"

namespace rrt_server
{
    class rrt_utility
    {
        public:

            inline double rad_to_deg(double rad) {return rad / M_PI * 180.0;}

            inline double deg_to_rad(double deg) {return deg / 180.0 * M_PI;}

            /** @brief Transform pose according to the translation and rpy given */
            inline Eigen::Vector3d transform_vector(
                Eigen::Vector3d p, Eigen::Vector3d rpy, Vector3d translation, 
                std::string forward_or_back)
            {
                double roll = deg_to_rad(rpy.x()), pitch = deg_to_rad(rpy.y()), yaw = deg_to_rad(rpy.z());    
                Eigen::Quaterniond q;
                q = AngleAxisd(roll, Eigen::Vector3d::UnitX())
                    * AngleAxisd(pitch, Eigen::Vector3d::UnitY())
                    * AngleAxisd(yaw, Eigen::Vector3d::UnitZ());

                Eigen::Quaterniond point;
                point.w() = 0;
                // To identify which one comes first the rotation or the translation
                if (forward_or_back.compare("backward")==0)
                {
                    point.vec() = Vector3d(p.x(), p.y(), p.z());
                    Eigen::Quaterniond rotatedP = q * point * q.inverse();
                    return rotatedP.vec() + translation;
                }
                else if (forward_or_back.compare("forward")==0)
                {
                    point.vec() = Vector3d(p.x(), p.y(), p.z()) - translation;
                    Eigen::Quaterniond rotatedP = q * point * q.inverse();
                    return rotatedP.vec();
                }
                else
                    return Eigen::Vector3d();           
            }

            /** @brief Filter/Crop point cloud with the dimensions given */
            inline pcl::PointCloud<pcl::PointXYZ>::Ptr pcl_ptr_box_crop(
                pcl::PointCloud<pcl::PointXYZ>::Ptr _pc, 
                Eigen::Vector3d centroid, Eigen::Vector3d dimension)
            {   
                pcl::PointCloud<pcl::PointXYZ>::Ptr output(
                    new pcl::PointCloud<pcl::PointXYZ>);

                Eigen::Vector3d min = centroid - (dimension / 2);
                Eigen::Vector3d max = centroid + (dimension / 2);

                pcl::CropBox<pcl::PointXYZ> box_filter;
                box_filter.setMin(Eigen::Vector4f(min.x(), min.y(), min.z(), 1.0));
                box_filter.setMax(Eigen::Vector4f(max.x(), max.y(), max.z(), 1.0));

                box_filter.setInputCloud(_pc);
                box_filter.filter(*output);

                return output;
            }

            /** @brief linspace function with given min and max **/
            inline vector<double> linspace(
                double min, double max, double n)
            {
                vector<double> linspaced;
                double delta = (max - min) / (n - 1.0);
                linspaced.push_back(min);
                
                for (int i = 1; i < (int)n; i++)
                {
                    linspaced.push_back(linspaced[i-1] + delta);
                }

                return linspaced;
            }

            inline bool kdtree_collide_pcl_bool(
                Eigen::Vector3d point, pcl::PointCloud<pcl::PointXYZ>::Ptr obs, double c)
            {
                pcl::KdTreeFLANN<pcl::PointXYZ> kdtree;

                kdtree.setInputCloud(obs);

                pcl::PointXYZ searchPoint;
                searchPoint.x = point.x();
                searchPoint.y = point.y();
                searchPoint.z = point.z();

                std::vector<int> pointIdxRadiusSearch;
                std::vector<float> pointRadiusSquaredDistance;

                // float radius = 256.0f * rand () / (RAND_MAX + 1.0f);

                float radius = (float)c;

                if ( kdtree.radiusSearch (searchPoint, radius, pointIdxRadiusSearch, pointRadiusSquaredDistance) > 0 )
                {
                    for (std::size_t i = 0; i < pointIdxRadiusSearch.size (); ++i)
                    {
                        return true;
                    }
                }

                return false;
            }

            inline int kdtree_collide_pcl_points_size(
                Eigen::Vector3d point, pcl::PointCloud<pcl::PointXYZ>::Ptr obs, double c)
            {
                pcl::KdTreeFLANN<pcl::PointXYZ> kdtree;

                kdtree.setInputCloud(obs);

                pcl::PointXYZ searchPoint;
                searchPoint.x = point.x();
                searchPoint.y = point.y();
                searchPoint.z = point.z();

                std::vector<int> pointIdxRadiusSearch;
                std::vector<float> pointRadiusSquaredDistance;

                // float radius = 256.0f * rand () / (RAND_MAX + 1.0f);

                float radius = (float)c;

                if ( kdtree.radiusSearch (searchPoint, radius, pointIdxRadiusSearch, pointRadiusSquaredDistance) > 0 )
                {
                    return (int)pointIdxRadiusSearch.size();
                }

                return 0;
            }

            inline Eigen::Vector3d rotate_translation_with_rpy(
                Eigen::Vector3d rotation, Eigen::Vector3d translation)
            {
                // https://github.com/felipepolido/EigenExamples
                // for affine3d examples
                double deg2rad = - 1.0 / 180.0 * M_PI;

                Eigen::Quaterniond q;
                q = AngleAxisd(rotation.x() * deg2rad, Vector3d::UnitX())
                    * AngleAxisd(rotation.y() * deg2rad, Vector3d::UnitY())
                    * AngleAxisd(rotation.z() * deg2rad, Vector3d::UnitZ());
                
                // w,x,y,z
                Eigen::Quaterniond rot(q.w(), q.x(), q.y(), q.z());
                rot.normalize();

                Eigen::Quaterniond p;
                p.w() = 0;
                p.vec() = - translation;
                Eigen::Quaterniond rotatedP = rot * p * rot.inverse(); 
                
                return rotatedP.vec();
            }

            /** @brief Check if the step node that has been generated is valid or not
             * It will be invalid if the step node either lies in the proximity of a pointcloud (that is, an obstacle) 
             * or if the straight line path joining nearest_node and step_node goes through an obstacle
            */
            inline bool check_line_validity_with_pcl(
                Eigen::Vector3d p, Eigen::Vector3d q, double obs_threshold,
                pcl::PointCloud<pcl::PointXYZ>::Ptr obs)
            {
                double distance = (p - q).norm();
                int n = ceil(distance / obs_threshold);

                Eigen::Vector3d large, small;

                vector<Eigen::Vector3d> line_vector;

                Eigen::Vector3d valid_origin = (p + q) / 2;

                Eigen::Vector3d abs_pq_vector = p - q;
                abs_pq_vector.x() = abs(abs_pq_vector.x());
                abs_pq_vector.y() = abs(abs_pq_vector.y());
                abs_pq_vector.z() = abs(abs_pq_vector.z());

                // Establish a kind of AABB box around the line
                Eigen::Vector3d local_map_size = abs_pq_vector + 
                    Eigen::Vector3d(2 * obs_threshold, 2 * obs_threshold, 2 * obs_threshold);

                pcl::PointCloud<pcl::PointXYZ>::Ptr local_obs;
                local_obs = pcl_ptr_box_crop(obs, valid_origin, local_map_size);

                size_t local_num_points = local_obs->size();
                int local_points_total = static_cast<int>(local_num_points);

                if (local_points_total == 0)
                    return true;

                vector<double> line_vector_x = linspace(p.x(), q.x(), (double)n);
                vector<double> line_vector_y = linspace(p.y(), q.y(), (double)n);
                vector<double> line_vector_z = linspace(p.z(), q.z(), (double)n);
                for (int i = 0; i < (int)line_vector_x.size(); i++)
                {
                    line_vector.push_back(Eigen::Vector3d(line_vector_x[i], line_vector_y[i], line_vector_z[i]));
                }

                Eigen::Vector3d clearance_crop = Eigen::Vector3d(2 * obs_threshold, 2 * obs_threshold, 2 * obs_threshold);

                for(int i = 0; i < (int)line_vector.size(); i++)
                {
                    Eigen::Vector3d point_in_line_vector = line_vector[i];
                    pcl::PointCloud<pcl::PointXYZ>::Ptr sub_local_obs;

                    // Check to see whether the point in the line 
                    // collides with the pointcloud

                    sub_local_obs = pcl_ptr_box_crop(local_obs, point_in_line_vector, clearance_crop);

                    size_t num_points = sub_local_obs->size();
                    int total = static_cast<int>(num_points);
                    
                    if (total == 0)
                        continue;

                    if (kdtree_collide_pcl_bool(point_in_line_vector, sub_local_obs, obs_threshold))
                        return false;

                }
                return true;
            }

    };


    class rrt_search_node
    {
        private:

        struct Node 
        {
            vector<Node *> children;
            Node *parent;
            Eigen::Vector3d position;
        };

        Node start_node, end_node;
        vector<Node*> nodes;
        pcl::PointCloud<pcl::PointXYZ>::Ptr obs;
        vector<Eigen::Vector4d> no_fly_zone;
        vector<Eigen::Vector3d> input; 

        rrt_server::rrt_utility ru; 

        double step_size;

        bool reached = false;
        int initialized = 0;
        double obs_threshold;
        int iter = 0;
        
        double _min_height, _max_height;

        double timeout = 0.1;

        Eigen::Vector3d map_size, origin;

        Eigen::Vector3d rotation, translation;

        std::random_device dev;

        void search_single_node()
        {
            std::mt19937 generator(dev());
            std::uniform_real_distribution<double> dis_middle(-1.0, 1.0);
            std::uniform_real_distribution<double> dis_normal(0.0, 1.0);
            
            Node* step_node = new Node;

            /** @brief Generate the random vector values */
            // No need no fly zone for random node since step node will handle it
            Eigen::Vector3d random_vector = 
                Eigen::Vector3d(dis_middle(generator), dis_middle(generator), dis_middle(generator));        

            int index = random_idx_selector(nodes.size());
            
            step_node->position = node_stepping(nodes[index]->position, random_vector);

            Eigen::Vector3d transformed_position = Vector3d(step_node->position.x(),
                step_node->position.y(), step_node->position.z());
            
            // To transform the point back to original frame
            Eigen::Vector3d original_position = ru.transform_vector(
                transformed_position, rotation, translation, "backward");
            
            for (int i = 0; i < (int)no_fly_zone.size(); i++)
            {
                // x_min, x_max, y_min, y_max in original frame
                double x_min = no_fly_zone[i][0], x_max = no_fly_zone[i][1];
                double y_min = no_fly_zone[i][2], y_max = no_fly_zone[i][3];
                
                // Reject point if it is in no fly zone
                if (original_position.x() <= x_max && original_position.x() >= x_min &&
                    original_position.y() <= y_max && original_position.y() >= y_min)
                {
                    return;
                }                    
            }

            bool flag = ru.check_line_validity_with_pcl(
                nodes[index]->position, step_node->position, obs_threshold, obs);

            if(!flag) return;

            // Add the new node into the list and push_back data on children and parent
            step_node->parent = nodes[index];
            nodes.push_back(step_node);
            nodes[index]->children.push_back(step_node);
            
            if((ru.check_line_validity_with_pcl(
                step_node->position, end_node.position, obs_threshold, obs)) && 
                (sq_separation(step_node->position, end_node.position) < step_size))
            {
                reached = true;
                end_node.parent = step_node;
                nodes.push_back(&end_node);
                (nodes[nodes.size()-1]->children).push_back(&end_node);
                return;
            }

            iter++;
        }

        int random_idx_selector(int nodes_size)
        {
            std::mt19937 generator(dev());
            std::uniform_real_distribution<double> dis_normal(0.0, 1.0);
            return (int)ceil(dis_normal(generator) * nodes_size) - 1;
        }

        // [separation] function takes two coordinates
        // as its input and returns the distance between them.
        double sq_separation(Eigen::Vector3d p, Eigen::Vector3d q)
        {
            Eigen::Vector3d v;
            v.x() = p.x() - q.x();
            v.y() = p.y() - q.y();
            v.z() = p.z() - q.z();
            return pow(v.x(), 2) + pow(v.y(), 2) + pow(v.z(), 2);
        }

        // [node_stepping] function takes the random vector generated and its random node in the tree 
        // as its input, and returns the coordinates of the step node. 
        // This function determines the step node by generating a new node at a distance 
        // of step_size from nearest node towards random node
        Eigen::Vector3d node_stepping(Eigen::Vector3d node, Eigen::Vector3d random_vector)
        {
            std::mt19937 generator(dev());
            std::uniform_real_distribution<double> dis_step(0.6, 1.0);    
            double random_value = dis_step(generator);

            Eigen::Vector3d distance_vector = random_value * step_size * random_vector;

            Eigen::Vector3d step = distance_vector + node;

            step.x() = min(max(step.x(), -map_size.x()/2), map_size.x()/2);
            step.y() = min(max(step.y(), -map_size.y()/2), map_size.y()/2);
            step.z() = min(max(step.z(), _min_height),_max_height);

            return step;
        }

        std::vector<Vector3d> path_extraction()
        {
            Node up, down;
            down = end_node;
            up = *(end_node.parent);
            std::vector<Vector3d> path;

            while(1)
            {
                path.push_back(down.position);
                if(up.parent == NULL)
                    break;
                up = *(up.parent);
                down = *(down.parent);
            }

            return path;
        }

        public:

        rrt_search_node(vector<Eigen::Vector3d> _input)
        {
            input = _input;
        }
    
        ~rrt_search_node(){}

        // Initialization requires alot of data
        // 1. initialize_start_end
        // 2. initialize_boundaries
        // 3. initialize_map_characteristics
        // 4. initialize_node_characteristics

        void initialize_start_end(Eigen::Vector3d _start, Eigen::Vector3d _end)
        {
            initialized++;

            start_node.position = _start;
            start_node.parent = NULL;

            nodes.push_back(&start_node);
            end_node.position = _end;
        }

        void initialize_boundaries(
            double min_height, double max_height, 
            vector<Eigen::Vector4d> _no_fly_zone)
        {
            initialized++;

            no_fly_zone = _no_fly_zone;
            _min_height = min_height;
            _max_height = max_height;
        }

        void initialize_map_characteristics(
            pcl::PointCloud<pcl::PointXYZ>::Ptr _pc, 
            Vector3d _map_size, Vector3d _origin)
        {
            initialized++;

            obs = _pc;
            map_size = _map_size;
            origin = _origin;
        }

        void initialize_node_characteristics(
            double _timeout, double _step_size, double _obs_threshold, 
            Vector3d _rotation, Vector3d _translation)
        {
            initialized++;

            timeout = _timeout;
            rotation = _rotation;
            translation = _translation;
            obs_threshold = _obs_threshold;
            step_size = _step_size;
        }

        std::vector<Vector3d> run_rrt_module()
        {
            if (initialized != 4)
                return std::vector<Vector3d>();

            time_point<std::chrono::system_clock> fail_timer_start = system_clock::now();
            
            bool error = false;
            while(!reached)
            {
                search_single_node();
                if (duration<double>(system_clock::now() - fail_timer_start).count() > timeout)
                {
                    error = true;
                    break;
                }
            }

            if (error)
                return std::vector<Vector3d>();

            return path_extraction();
        }

    };
}

#endif