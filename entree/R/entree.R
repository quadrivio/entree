#
#  entree.R
#  entree
#
#  Created by MPB on 5/2/13.
#  Copyright (c) 2013 Quadrivio Corporation. All rights reserved.
#  License http://opensource.org/licenses/BSD-2-Clause
#          <YEAR> = 2013
#          <OWNER> = Quadrivio Corporation
#

entree <-
function(x, y, maxDepth = 500, minDepth = 1, maxTrees = 1000, columnsPerTree = NA, doRandom = FALSE,
doPrune = FALSE, minImprovement = 0.0, minLeafCount = 4, maxSplitsPerNumericAttribute = -1,
xValueTypes = NA, yValueType = NA, xImputeOptions = NA)
{
    # make sure types are correct before calling C function
    
    # x
    x = data.frame(x)
    
    # y
    y = as.vector(y)
    # TODO if y is a factor, keep it that way instead of converting
    
    # maxDepth
    storage.mode(maxDepth) <- "integer"
    
    # minDepth
    storage.mode(minDepth) <- "integer"

    # maxTrees
    storage.mode(maxTrees) <- "integer"

    # columnsPerTree
    if (is.na(columnsPerTree)) {
        # determine automatically
        columnsPerTree = 0
    }
    storage.mode(columnsPerTree) <- "integer"

    # doRandom
    storage.mode(doRandom) <- "integer"

    # doPrune
    storage.mode(doPrune) <- "integer"

    # minImprovement
    storage.mode(minImprovement) <- "double"
    
    # minLeafCount
    storage.mode(minLeafCount) <- "integer"
    
    # maxSplitsPerNumericAttribute
    storage.mode(maxSplitsPerNumericAttribute) <- "integer"
    
    # xValueTypes
    if (is.na(xValueTypes[1])) {
        xValueTypes = c()
    }
    storage.mode(xValueTypes) <- "character"

    # yValueType
    if (is.na(yValueType)) {
        yValueType = c()
    }
    storage.mode(yValueType) <- "character"

    # xImputeOptions
    if (is.na(xImputeOptions[1])) {
        xImputeOptions = c()
    }
    storage.mode(xImputeOptions) <- "character"

	z = .Call(entree_C, x, y, maxDepth, minDepth, maxTrees, columnsPerTree, doRandom, doPrune,
            minImprovement, minLeafCount, maxSplitsPerNumericAttribute, xValueTypes, yValueType,
            xImputeOptions)
    
    # add other input parameters to object
    result = z
    result$maxDepth = maxDepth
    result$minDepth = minDepth
    result$maxTrees = maxTrees
    result$doRandom = doRandom
    result$doPrune = doPrune
    result$minImprovement = minImprovement
    result$minLeafCount = minLeafCount
    result$maxSplitsPerNumericAttribute = maxSplitsPerNumericAttribute

	return(result)
}
