# Rclusterpp -- Large-scale hierarchical clustering in R

Rclusterpp provides flexible native hierarchical clustering routings optimized
for performance and minimal memory requirements. In particular Rclusterpp
includes "stored data" clustering implementations with *O(n)* memory
footprints. Rclusterpp has been successfully used to cluster 100,000s of observations.

Rclusterpp makes extensive use of
[Rcpp](http://dirk.eddelbuettel.com/code/rcpp.html) for integration with R, and
the [Eigen](http://eigen.tuxfamily.org) matrix library (via
[RcppEigen](http://cran.r-project.org/web/packages/RcppEigen/index.html)).
Rclusterpp provides a R interface to its internal libraries that can be used in
place of `stats::hclust` and provides linkable libraries for use by downstream packages. 