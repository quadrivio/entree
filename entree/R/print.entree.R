#
#  print.entree.R
#  entree
#
#  Created by MPB on 5/2/13.
#  Copyright (c) 2013 Quadrivio Corporation. All rights reserved.
#  License http://opensource.org/licenses/BSD-2-Clause
#          <YEAR> = 2013
#          <OWNER> = Quadrivio Corporation
#

print.entree <-
function(x, ...)
{
    # print brief summary
	cat("maxDepth", x$maxDepth, "\n")
	cat("minDepth", x$minDepth, "\n")
	cat("maxTrees", x$maxTrees, "\n")
	cat(length(x$trees), "tree(s)", "\n")
}
