#include "Okno.h"

using namespace System;
using namespace System::Windows::Forms;

[STAThreadAttribute]
int main(array<String^>^ args) {
    system("chcp 1251 > nul");
    Application::EnableVisualStyles();
    Application::SetCompatibleTextRenderingDefault(false);

    Monitoringseti::Okno form;
    Application::Run(% form);

    return 0;
}

