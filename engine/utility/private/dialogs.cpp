#include "dialogs.hpp"

#include <filesystem>
#include <minwinbase.h>
#include <minwindef.h>
#include <commdlg.h>

static std::filesystem::path OpenFileDialog(Filters filters)
{
    OPENFILENAME ofn;
    char szFileName[MAX_PATH] = "";

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn); // SEE NOTE BELOW
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = filters
                          ofn.lpstrFile
        = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "txt";

    if (GetOpenFileName(&ofn))
    {
        // Do something usefull with the filename stored in szFileName
    }

    return {};
}