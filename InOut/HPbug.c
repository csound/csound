/*	Csound will not run entirely without error on HP computers	*/
/*	using the 68881 floating co-processor unless the compiler	*/
/*	will allow the following code to run correctly:			*/

main()
{
	float a = 0., b = 1., c;

	b *= b;
	if ((c = a) == 0.)
		printf("test for 0 ok\n");
	else printf("test for 0 failed\n");
}
