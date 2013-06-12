\name{entree}
\alias{entree}
\alias{print.entree}
\alias{predict.entree}
\title{Ensemble of decision trees}
\description{
An implementation of decision tree ensembles, using binary splits and a deterministic
  (non-random) set of columns for each tree.
}
\usage{
entree(x, y, maxDepth = 500, minDepth = 1, maxTrees = 1000, columnsPerTree = NA, doPrune = FALSE,
  minImprovement = 0.0, minLeafCount = 4, maxSplitsPerNumericAttribute = -1, xValueTypes = NA,
  yValueType = NA, xImputeOptions = NA)
\method{print}{entree}(x, \dots)
\method{predict}{entree}(object, x, \dots)
}

\arguments{
  \item{x}{input matrix or data.frame }
  \item{y}{output vector }
  \item{maxDepth}{maximum depth of each tree}
  \item{minDepth}{minimum depth of tree to be used in ensemble}
  \item{maxTrees}{maximum number of trees in ensemble}
  \item{columnsPerTree}{number of columns per tree}
  \item{doPrune}{prune trees after building (experimental)}
  \item{minImprovement}{minimum fractional improvement for splitting numerical value}
  \item{minLeafCount}{minimum count of examples in a leaf}
  \item{maxSplitsPerNumericAttribute}{max times to split a numeric attribute in one path}
  \item{xValueTypes}{vector of x column types: "c" = categorical, "n" = numerical}
  \item{yValueType}{y type: "c" = categorical, "n" = numerical}
  \item{xImputeOptions}{vector of x imputation options: "category", "mode", "mean", "median"}
  \item{object}{object of class "entree"}
  \item{...}{other stuff}
}
\value{
Returns object of class "entree"}
\author{
Michael Budiansky, Quadrivio Corporation
}
\examples{
x = matrix(c(1,2,3, 11,12,13), nrow = 2, ncol=3, byrow=TRUE, dimnames = list(NULL, c("C.1", "C.2", "C.3")))
y = c(56, 86)
fit.entree = entree(x, y = y, minLeafCount = 1)
print(fit.entree)
predict(fit.entree, x)

yc = c("cat", "dog")
fit.entree = entree(x, y = yc, minLeafCount = 1)
print(fit.entree)
predict(fit.entree, x)

dfx = data.frame(x)
xf = as.factor(c("foo", "bar"))
yf = as.factor(yc)
dfx = cbind(dfx, xf)
fit.entree = entree(dfx, y = yf, minDepth = 0, minLeafCount = 1)
print(fit.entree)
predict(fit.entree, dfx)
}

\keyword{classif}
\keyword{regression}
\keyword{tree}
