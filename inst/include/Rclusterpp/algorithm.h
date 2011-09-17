#ifndef RCLUSTERP_ALGORITHM_H
#define RCLUSTERP_ALGORITHM_H

#include <limits>
#include <utility>
#include <algorithm>
#include <functional>
#include <vector>

#include <Rclusterpp/cluster.h>

namespace Rclusterpp {

	namespace {
			
		template<class OP>
		class binder1st_P : public std::unary_function<typename OP::second_argument_type*, typename OP::result_type> {
		public:
			binder1st_P(OP& o, typename OP::first_argument_type const* a1) : op(o), arg(a1) {}
			typename OP::result_type operator()(const typename OP::second_argument_type* x) {
				return op(*arg, *x);
			}
			
		private:
			OP& op;
			typename OP::first_argument_type const* arg;
		};

		template<class OP>
		inline binder1st_P<OP> bind1st_P(OP& o, typename OP::first_argument_type const* a) { return binder1st_P<OP>(o,a); } 
		
	} // end of anonymous namespace

	
	template<class ForwardIterator, class Distancer>
	std::pair<ForwardIterator, typename Distancer::result_type> nearest_neighbor(
		ForwardIterator first, 
		ForwardIterator last, 
		Distancer       distancer, 
		typename Distancer::result_type max_dist=std::numeric_limits<typename Distancer::result_type>::max()
	) {
	
		typedef typename Distancer::result_type Dist_t;

		ForwardIterator min_i = last;
		Dist_t          min_d = max_dist;
		for (ForwardIterator i=first; i!=last; ++i) {
			Dist_t dist = distancer(*i);
			if (dist < min_d) {
				min_i = i;
				min_d = dist;
			}
		}

		return std::make_pair(min_i, min_d);	
	}
	

	template<class ClusteringMethod, class ClusterVector>
	void cluster_via_rnn(ClusteringMethod method, ClusterVector& clusters) {

		typedef ClusterVector                                            clusters_type;
		typedef typename Traits::Collection<clusters_type>::cluster_type cluster_type;
		typedef typename ClusteringMethod::distance_type                 distance_type;
	
		// Result from nearest neighbor scan
		typedef std::pair<typename clusters_type::iterator, distance_type>  nearn_type;

#define nn_cluster(x) (x).first
#define distance_to_nn(x) (x).second

		// Nearest neighbor chain
		typedef std::pair<typename clusters_type::value_type, distance_type> entry_type;

		std::vector<entry_type> chain;

#define cluster_at_tip(x) (x).back().first
#define distance_to_tip(x) (x).back().second

						
		// Expand the size of clusters vector to the contain exactly the newly created clusters	
		size_t initial_clusters = clusters.size(), result_clusters = (initial_clusters * 2) - 1;
		clusters.reserve(result_clusters);
		
		typename clusters_type::iterator next_unchained = clusters.begin();
		while (clusters.size() != result_clusters) {
			if (chain.empty()) {
				// Pick next "unchained" cluster as default
				chain.push_back( entry_type(*next_unchained, std::numeric_limits<distance_type>::max()) );
				++next_unchained;
			} else {

				// Find next nearest neighbor from remaining "unchained" clusters			
				nearn_type nn = nearest_neighbor(
					next_unchained, 
					clusters.end(), 
					bind1st_P(method.distancer, cluster_at_tip(chain)), // Bind tip into distance function for computing nearest neighbor 
					distance_to_tip(chain)
				);
				
				if (nn.first != clusters.end()) {
					std::iter_swap(next_unchained, nn_cluster(nn));
					chain.push_back( entry_type(*next_unchained, distance_to_nn(nn)) );
					++next_unchained;
				} else {
					// Tip of chain is recursive nearest neighbor
					cluster_type* r = cluster_at_tip(chain);
					distance_type d = distance_to_tip(chain);
					chain.pop_back();

					cluster_type* l  = cluster_at_tip(chain);
					chain.pop_back();

					// Remove "tip" and "next tip"  from chain and merge into new cluster appended to "unchained" clusters
					cluster_type* cn = new cluster_type(l, r, d);
					method.merger(*cn, *(cn->parent1()), *(cn->parent2()));
					clusters.push_back(cn);
				}
			}
		}

		

		// Cleanup cluster listing
		
		// Re-order the clusters, with initial clusters in the beginning, ordered by id
		// from 0 .. -initial_clusters, followed by the agglomerated clusters sorted by
		// increasing disimilarity.
		
		std::partition(clusters.begin(), clusters.end(), std::mem_fun(&cluster_type::initial));
		std::sort(clusters.begin(), clusters.begin()+initial_clusters, std::not2(std::ptr_fun(&compare_id<cluster_type>))); 
		std::sort(clusters.begin() + initial_clusters, clusters.end(), std::ptr_fun(&compare_disimilarity<cluster_type>)); 

		for (size_t i=initial_clusters; i<result_clusters; i++) {
			clusters[i]->set_id(i - initial_clusters + 1);  // Use R hclust 1-indexed convention for Id's
		}

	}

} // end of Rclustercpp namespace

#endif