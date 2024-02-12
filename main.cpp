#include "pch.h"

#include <iostream>
#include <winerror.h>
#include <winrt/Windows.Storage.h>
#include "RBOperator.h"
#include <io.h>
#include <fcntl.h>

#pragma execution_character_set( "utf-8" )

using namespace winrt;
using namespace Windows::Foundation;

namespace WS = winrt::Windows::Storage;

IAsyncOperation<int> deleteFile(hstring filePath)
{
    try
    {
        const auto fileToDelete = co_await WS::StorageFile::GetFileFromPathAsync(filePath);
        co_await fileToDelete.DeleteAsync(WS::StorageDeleteOption::Default);
    }
    catch (const hresult_error& error)
    {
        co_return error.code();
    }
}

IAsyncOperation<int> moveFile(hstring filePath, hstring destinationFolderPath, hstring desiredName)
{
    try
    {
        const auto fileToMove = co_await WS::StorageFile::GetFileFromPathAsync(filePath);
        const auto destinationFolder = co_await WS::StorageFolder::GetFolderFromPathAsync(destinationFolderPath);
        co_await fileToMove.MoveAsync(destinationFolder, desiredName, WS::NameCollisionOption::GenerateUniqueName);
    }
    catch (const hresult_error& error)
    {
        co_return error.code();
    }
}

int main()
{
    init_apartment();
    
    /*std::wcout << L"File to delete (full path): ";

    std::wstring fileToDelete;
    std::getline(std::wcin, fileToDelete);

    long ret = deleteFile(winrt::to_hstring(fileToDelete.c_str())).get();

    if (ret != ERROR_SUCCESS)
    {
        std::wcout << std::system_category().message(ret).c_str() << std::endl;
        return 0;
    }

    std::wcout << L"File deleted." << std::endl;*/

    _setmode(_fileno(stdout), _O_U16TEXT);

    std::wcout << std::endl << L"Listing files in Recycle Bin..." << std::endl;
    std::wcout << L"-------------------------" << std::endl;

    RBOperator rb;
    rb.Init();
    auto allFileInfo = rb.GetAllFileInfo();

    ULONG l = 0;
    for (auto& info : allFileInfo)
    {
        std::wcout << l << L". " << info.originalName << L"  " << info.originalFolder << L" [" << info.dateDeleted << L"] " << info.currentFullPath << std::endl;
        ++l;
    }

    std::wcout << std::endl << L"Enter nth file to restore (0, 1, etc.): " << std::endl;

    ULONG nthFile;
    std::wcin >> nthFile;

    int ret = rb.UndeleteNthFileAsync(allFileInfo, nthFile).get();

    if (ret != ERROR_SUCCESS)
    {
        std::wcout << std::system_category().message(ret).c_str() << std::endl;
        return 0;
    }

    std::wcout << allFileInfo[nthFile].originalName.c_str() << L" restored." << std::endl;

    return 0;
}
