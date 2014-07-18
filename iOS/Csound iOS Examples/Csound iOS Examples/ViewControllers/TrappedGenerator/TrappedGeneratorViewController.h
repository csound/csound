//
//  TrappedGeneratorViewController.h
//  Csound iOS Examples
//
//  Created by Steven Yi on 1/23/13.
//
//

#import "BaseCsoundViewController.h"

@interface TrappedGeneratorViewController : BaseCsoundViewController<CsoundObjListener> 

-(IBAction)generateTrappedToDocumentsFolder:(id)sender;

@end
