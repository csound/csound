#include <stdio.h>
#include <math.h>

double round(double);

int main(int argc, char **argv)
{
  double pixels_per_inch = 72.27;
  double mag = 0.0;
  if (argc > 1) {
    int n = sscanf(argv[1], "%lf", &mag);
    if (n != 1)
      return 1;
  }
  mag = round(pixels_per_inch * mag);
  printf("%.lf\n", mag);
  return 0;
}
