#include "pch.h"
#include <iostream>
#include <shlobj_core.h>
#include <shlwapi.h>
#include "RBOperator.h"

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Shlwapi.lib")

RBOperator::RBOperator()
{
	m_psfRecycleBin = nullptr;
	m_peidl = nullptr;
}

RBOperator::~RBOperator()
{
	if (nullptr != m_peidl)
	{
		m_peidl->Release();
		m_peidl = nullptr;
	}

	if (nullptr != m_psfRecycleBin)
	{
		m_psfRecycleBin->Release();
		m_psfRecycleBin = nullptr;
	}
}

void RBOperator::Init()
{
	PIDLIST_ABSOLUTE pidl;
	HRESULT hr = SHGetKnownFolderIDList(FOLDERID_RecycleBinFolder, KF_FLAG_DEFAULT, NULL, &pidl);
	if (SUCCEEDED(hr))
	{
		hr = SHBindToObject(NULL, pidl, NULL, IID_PPV_ARGS(&m_psfRecycleBin));
		CoTaskMemFree(pidl);

		if (SUCCEEDED(hr))
		{
			hr = m_psfRecycleBin->EnumObjects(NULL, SHCONTF_NONFOLDERS, &m_peidl);
		}
	}

	if (!SUCCEEDED(hr))
	{
		throw winrt::hresult_error(hr, winrt::to_hstring(L""));
	}

	if (nullptr == m_peidl)
	{
		throw winrt::hresult_error(E_FAIL, winrt::to_hstring(L"Unable to obtain IEnumIDList of Recyle Bin"));
	}
}

std::vector<RBOperator::DeletedFileInfo> RBOperator::GetAllFileInfo()
{
	PITEMID_CHILD pidlItem = nullptr;

	m_peidl->Reset();

	std::vector<RBOperator::DeletedFileInfo> allFileInfo;

	while (m_peidl->Next(1, &pidlItem, NULL) == S_OK)
	{
		RBOperator::DeletedFileInfo fileInfo
		{
			GetFullPathInRecycleBin(m_psfRecycleBin, pidlItem),
			GetOriginalName(m_psfRecycleBin, pidlItem),
			GetOriginalLocation(m_psfRecycleBin, pidlItem),
			GetDeletedDate(m_psfRecycleBin, pidlItem)
		};

		CoTaskMemFree(pidlItem);

		if (!(fileInfo.currentFullPath.empty() || fileInfo.originalName.empty() || fileInfo.originalFolder.empty()))
		{
			allFileInfo.push_back(fileInfo);
		}
	}

	return allFileInfo;
}

IAsyncOperation<int> RBOperator::UndeleteNthFileAsync(const std::vector<RBOperator::DeletedFileInfo>& allFileInfo, ULONG nth)
{
	assert(allFileInfo.size() > nth);
	DeletedFileInfo fileInfo = allFileInfo[nth];
	return co_await UndeleteFile(winrt::to_hstring(fileInfo.currentFullPath.c_str()),
				winrt::to_hstring(fileInfo.originalFolder.c_str()),
				winrt::to_hstring(fileInfo.originalName.c_str()));
}

std::wstring RBOperator::getDisplayNameOf(IShellFolder* psf, PCUITEMID_CHILD pidl, SHGDNF uFlags)
{
	STRRET sr;
	HRESULT hr = psf->GetDisplayNameOf(pidl, uFlags, &sr);
	if (SUCCEEDED(hr))
	{
		LPWSTR pszName;
		hr = StrRetToStrW(&sr, pidl, &pszName);
		if (SUCCEEDED(hr))
		{
			std::wstring ret(pszName);
			CoTaskMemFree(pszName);
			return ret;
		}
	}
	return std::wstring();
}

std::wstring RBOperator::getDetail(IShellFolder2* psf, PCUITEMID_CHILD pidl, const SHCOLUMNID* pscid)
{
	VARIANT vt;
	std::wstring ret;
	HRESULT hr = psf->GetDetailsEx(pidl, pscid, &vt);
	if (SUCCEEDED(hr))
	{
		hr = VariantChangeType(&vt, &vt, 0, VT_BSTR);
		if (SUCCEEDED(hr))
		{
			ret = V_BSTR(&vt);
		}
		VariantClear(&vt);
	}
	return ret;
}

std::wstring RBOperator::GetOriginalName(IShellFolder* psf, PCUITEMID_CHILD pidl)
{
	return getDisplayNameOf(psf, pidl, SHGDN_INFOLDER);
}

std::wstring RBOperator::GetFullPathInRecycleBin(IShellFolder* psf, PCUITEMID_CHILD pidl)
{
	return getDisplayNameOf(psf, pidl, SHGDN_FORPARSING);
}

std::wstring RBOperator::GetOriginalLocation(IShellFolder2* psf, PCUITEMID_CHILD pidl)
{
	const SHCOLUMNID SCID_OriginalLocation = { PSGUID_DISPLACED, PID_DISPLACED_FROM };
	return getDetail(psf, pidl, &SCID_OriginalLocation);
}

std::wstring RBOperator::GetDeletedDate(IShellFolder2* psf, PCUITEMID_CHILD pidl)
{
	const SHCOLUMNID SCID_DateDeleted = { PSGUID_DISPLACED, PID_DISPLACED_DATE };
	return getDetail(psf, pidl, &SCID_DateDeleted);
}

PITEMID_CHILD RBOperator::GetNthItemID(IEnumIDList* peidl, ULONG nth)
{
	PITEMID_CHILD pidlItem = nullptr;
	peidl->Reset();
	peidl->Skip(nth);
	m_peidl->Next(1, &pidlItem, NULL);
	return pidlItem;
}

IAsyncOperation<int> RBOperator::UndeleteFile(winrt::hstring filePath, winrt::hstring destinationFolderPath, winrt::hstring desiredName)
{
	try
	{
		const auto fileToMove = co_await WS::StorageFile::GetFileFromPathAsync(filePath);
		const auto destinationFolder = co_await WS::StorageFolder::GetFolderFromPathAsync(destinationFolderPath);
		co_await fileToMove.MoveAsync(destinationFolder, desiredName, WS::NameCollisionOption::GenerateUniqueName);
	}
	catch (const winrt::hresult_error& error)
	{
		co_return error.code();
	}
}