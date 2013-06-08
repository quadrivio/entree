#
#  predict.entree.R
#  entree
#
#  Created by MPB on 5/2/13.
#  Copyright (c) 2013 Quadrivio Corporation. All rights reserved.
#  License http://opensource.org/licenses/BSD-2-Clause
#          <YEAR> = 2013
#          <OWNER> = Quadrivio Corporation
#

predict.entree <-
function(object, x, ...)
{
    x = data.frame(x)

	y = .Call(entree_predict_C, object, x)

	return(y)
}
