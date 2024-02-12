#pragma once

#include <shobjidl_core.h>
#include <winrt/Windows.Storage.h>

using namespace winrt::Windows::Foundation;
namespace WS = winrt::Windows::Storage;

class RBOperator
{
private:
	IShellFolder2* m_psfRecycleBin;
	IEnumIDList* m_peidl;

	struct DeletedFileInfo
	{
		std::wstring currentFullPath;
		std::wstring originalName;
		std::wstring originalFolder;
		std::wstring dateDeleted;
	};

public:
	RBOperator();
	~RBOperator();

	void Init();
	std::vector<DeletedFileInfo> GetAllFileInfo();
	IAsyncOperation<int> UndeleteNthFileAsync(const std::vector<RBOperator::DeletedFileInfo>&, ULONG);	// nth starts from 0

private:
	std::wstring GetFullPathInRecycleBin(IShellFolder* psf, PCUITEMID_CHILD pidl);
	std::wstring GetOriginalName(IShellFolder* psf, PCUITEMID_CHILD pidl);
	std::wstring GetOriginalLocation(IShellFolder2* psf, PCUITEMID_CHILD pidl);
	std::wstring GetDeletedDate(IShellFolder2* psf, PCUITEMID_CHILD pidl);
	PITEMID_CHILD GetNthItemID(IEnumIDList*, ULONG);	// nth starts from 0
	IAsyncOperation<int> UndeleteFile(winrt::hstring filePath, winrt::hstring destinationFolderPath, winrt::hstring desiredName);
	std::wstring getDisplayNameOf(IShellFolder* psf, PCUITEMID_CHILD pidl, SHGDNF uFlags);
	std::wstring getDetail(IShellFolder2* psf, PCUITEMID_CHILD pidl, const SHCOLUMNID* pscid);
};

