\name{entree-package}
\alias{entree-package}
\docType{package}
\title{Ensemble of decision trees}
\description{
An implementation of decision tree ensembles, using binary splits and a deterministic
  (non-random) set of columns for each tree.
}
\details{
\tabular{ll}{
Package: \tab entree\cr
Type: \tab Package\cr
Version: \tab 0.10-3\cr
Date: \tab 2015-04-09\cr
License: \tab BSD + file LICENSE\cr
}
This package creates and uses an ensemble of decision trees for classification and regression. To
create the ensemble, call entree() with training data consisting of a data.frame containing the
values of attributes, and a vector of responses. This function returns an object that encapsulates
the information needed to make predictions. To predict the outcomes from a new set of attribute
data, call predict() with the object and a data.frame containing the values of attributes; the
function returns a vector of predictions.

By default, columns of factors or of character type are assumed to represent catgorical data, and
columns of numeric type are assumed to represent numeric data. To override this, supply a vector
of strings for xValueTypes and/or a single string for yValueType where "c" specifies categorical
and "n" specifies numeric.

Also by default, missing (NA) values are automatically imputed by choosing the median value of the
training data for numeric attributes, and by creating an additional category for missing data for
categorical attributes. To override this, supply a vector of strings for xImputeOptions where the
options are "mode" or "category" for categorical attributes, and "median" or "mean" for numeric
attributes; if "none" is specified, then missing numeric values are considered to be zero and missing
categorical values are considered to be an unknown category that doesn't match any existing
category; if "default" is specified, then "category" is used for categorical attributes and "median"
is used for numeric attributes.

If columnsPerTree is not specified, the number of columns per tree is set to 1/3 of the total number
of attribute columns when predicting a numeric result (regression), or the square root of the total
number when predicting a categorical result (classification). To create a single decision tree using
all the attributes, specify maxTrees = 1 and columnsPerTree = <total number of attribute columns>.
}
\author{
Michael Budiansky, Quadrivio Corporation
software@quadrivio.com
}

\keyword{package}
\keyword{classif}
\keyword{regression}
\keyword{tree}

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
