//###############################################################
// RegistationStatistics.h
// Laurent MEUNIER
// $date
//
// structure for registration quality related features
// computed in ICP.h
//###############################################################

#ifndef REGISTRATION_STATISTICS_H
#define REGISTRATION_STATISTICS_H
extern void fitPnt3Plane(const vector<Pnt3>&,Pnt3&,float&,Pnt3&);


class RegistrationStatistics {
 public:
  enum {
			PERCENTAGE_OVERLAPPING      = 1,
			AVG_MOTION                  = 2,
			CONDITION_NUMBER            = 3,
			PAIR_STABILITY              = 4,
			RMS_POINT_TO_PLANE_ERROR    = 5,
			RMS_POINT_TO_POINT_ERROR    = 6
  };


  bool discarded;

  float percentageOverlapping;
  float percentageKept;
  float avgConfidence;

  float avgMotion;
  float conditionNumber;

  float RMSPointToPointError;
  float AVGPointToPointError;
  float pointToPointDeviation;

  float RMSPointToPlaneError;
  float AVGPointToPlaneError;
  float pointToPlaneDeviation;

  float mainDirProjections[8];


  // init everything
  RegistrationStatistics() {
    discarded = false;

    percentageOverlapping = 0.0;
    percentageKept = 0.0;
    avgConfidence = 0.0;

    avgMotion = 0.0;
    conditionNumber = 0.0;

    RMSPointToPointError = 0.0;
    AVGPointToPointError = 0.0;
    pointToPointDeviation = 0.0;

    RMSPointToPlaneError = 0.0;
    AVGPointToPlaneError = 0.0;
    pointToPlaneDeviation = 0.0;

    for(int i = 0; i < 8; i++)
      mainDirProjections[i] = 0.0;
  }


  bool test(RegistrationStatistics t) {
    return (
	(percentageOverlapping > t.percentageOverlapping) &&
	(percentageKept > t.percentageKept) &&
	(avgMotion < t.avgMotion) &&
	(RMSPointToPlaneError < t.RMSPointToPlaneError) &&
	(RMSPointToPointError < t.RMSPointToPointError)
	);
  }

  float getResult(int resultType) {
    switch(resultType) {
    case PERCENTAGE_OVERLAPPING : return percentageOverlapping;
    case PAIR_STABILITY : return percentageKept;
    case AVG_MOTION : return -avgMotion;
    case CONDITION_NUMBER : return -conditionNumber;
    case RMS_POINT_TO_PLANE_ERROR : return -RMSPointToPlaneError;
    case RMS_POINT_TO_POINT_ERROR : return -RMSPointToPointError;
    }
    return 0;
  }

  // print everything
  void print() {
    if(discarded) {
      cout << "this pairing was discarded (overlapping area is to small)\n";
      return;
    }


    cout << "overlapping = " << percentageOverlapping << "%\n";
    cout << "kept = " << percentageKept << "%\n";
    cout << "avg confidence = " << avgConfidence << "\n";
    cout << "avg motion = " << avgMotion << "mm\n";
    cout << "condition number = " << conditionNumber << "\n";
    cout << "point to point: avg = " << AVGPointToPointError
         << "mm, rms = " << RMSPointToPointError
         << "mm,  deviation = " << pointToPointDeviation << "mm\n";
    cout << "point to plane: avg = " << AVGPointToPlaneError
	 << "mm, rms = " << RMSPointToPlaneError
         << "mm,  deviation = " << pointToPlaneDeviation << "mm\n";

    char str1[80], str2[80], str3[80];
    sprintf(str1, "projections: %9.3f %9.3f %9.3f\n", mainDirProjections[3], mainDirProjections[2], mainDirProjections[1]);
    sprintf(str2, "             %9.3f           %9.3f\n", mainDirProjections[4], mainDirProjections[0]);
    sprintf(str3, "             %9.3f %9.3f %9.3f\n", mainDirProjections[5], mainDirProjections[6], mainDirProjections[7]);
    cout << str1 << str2 << str3 << "\n" << flush;
  }

  static void computeMainDirections(vector<Pnt3> pts, Pnt3 *directions, Pnt3 &centroid) {
    Pnt3 norm;
    float d;

    fitPnt3Plane(pts, norm, d, centroid);
    Pnt3 u = cross(norm, Pnt3(1, 0, 0));
    norm.normalize();
    float lu = u.norm();
    if(lu == 0.0)
      u = cross(norm, Pnt3(0, 1, 0));
    u.normalize();
    Pnt3 v = cross(norm, u);
    v.normalize();

    directions[0] = u;
    directions[1] = (u + v).normalize();
    directions[2] = v;
    directions[3] = (u*(-1) + v).normalize();
    directions[4] = u*(-1);
    directions[5] = (u*(-1) + v*(-1)).normalize();
    directions[6] = v*(-1);
    directions[7] = (u + v*(-1)).normalize();
  }

  static float projection(vector<Pnt3> pts, Pnt3 centroid, Pnt3 direction) {
    float sum = 0;
    for(int i = 0; i < pts.size(); i++) {
      float d = dot(pts[i]-centroid, direction);
      sum += (d>0)?d:0.0;
    }
    return sum / pts.size();
  }

};


#endif
