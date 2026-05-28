{
    TCanvas *Hobbes = new TCanvas("Dist","Rod Timing Characteristics",
                                  5,5,1200,600);
    //Hobbes->cd();
    Hobbes->SetGrid();
    Hobbes->Divide(0,2);
    //TPad    *Calvin = new TPad("Calvin", "Jitter", 0.01, 0.01 ,0.99,0.99);
    //Calvin->Draw();
    Hobbes->cd(1);
    gPad->SetGrid();

    char *Names[] = {"Sec", "M", "D", "Y", "H","M","S", 
                     "Delta", "AVGdt","NSet", "dtNOW"};
    TNtuple *Rod = new TNtuple("RodDelta", "Timing on Rod", Names);
    Rod->ReadFile("oncore.log");
    Rod->Draw("AVGdt:Sec");

    Hobbes->cd(2);
    gPad->SetGrid();
    Rod->Draw("dtNOW");
}
