﻿/*
filetype.cpp

Работа с ассоциациями файлов
*/
/*
Copyright © 1996 Eugene Roshal
Copyright © 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "filetype.hpp"

#include "keys.hpp"
#include "dialog.hpp"
#include "vmenu.hpp"
#include "vmenu2.hpp"
#include "preservelongname.hpp"
#include "ctrlobj.hpp"
#include "cmdline.hpp"
#include "history.hpp"
#include "filemasks.hpp"
#include "message.hpp"
#include "interf.hpp"
#include "config.hpp"
#include "execute.hpp"
#include "fnparce.hpp"
#include "configdb.hpp"
#include "lang.hpp"
#include "DlgGuid.hpp"
#include "global.hpp"

#include "platform.fs.hpp"

/* $ 14.01.2001 SVS
   Добавим интеллектуальности.
   Если встречается "IF" и оно выполняется, то команда
   помещается в список

   Вызывается для F3, F4 - ассоциации
   Enter в ком строке - ассоциации.
*/
/* $ 06.07.2001
   + Используем filemasks вместо GetCommaWord, этим самым добиваемся того, что
     можно использовать маски исключения
   - Убрал непонятный мне запрет на использование маски файлов типа "*.*"
     (был когда-то, вроде, такой баг-репорт)
*/
bool ProcessLocalFileTypes(string_view const Name, string_view const ShortName, FILETYPE_MODE Mode, bool AlwaysWaitFinish, bool AddToHistory, bool RunAs, function_ref<void(execute_info&)> const Launcher)
{
	string strCommand, strDescription;

	const subst_context Context(Name, ShortName);

	{
		int ActualCmdCount=0; // отображаемых ассоциаций в меню
		filemasks FMask; // для работы с масками файлов

		int CommandCount=0;

		std::vector<MenuItemEx> MenuItems;

		for(const auto& [Id, Mask]: ConfigProvider().AssocConfig()->TypedMasksEnumerator(Mode))
		{
			strCommand.clear();

			if (FMask.Set(Mask, FMF_SILENT))
			{
				if (FMask.Compare(Context.Name))
				{
					ConfigProvider().AssocConfig()->GetCommand(Id, Mode, strCommand);

					if (!strCommand.empty())
					{
						ConfigProvider().AssocConfig()->GetDescription(Id, strDescription);
						CommandCount++;
					}
				}

				if (strCommand.empty())
					continue;
			}

			string strCommandText = strCommand;
			if (
				!SubstFileName(strCommandText, Context, nullptr, nullptr, true) ||
				// все "подставлено", теперь проверим условия "if exist"
				!ExtractIfExistCommand(strCommandText)
			)
				continue;

			ActualCmdCount++;

			if (!strDescription.empty())
				SubstFileName(strDescription, Context, nullptr, nullptr, true, {}, true);
			else
				strDescription = strCommandText;

			MenuItemEx TypesMenuItem(strDescription);
			TypesMenuItem.ComplexUserData = strCommand;
			MenuItems.push_back(std::move(TypesMenuItem));
		}

		if (!CommandCount)
			return false;

		if (!ActualCmdCount)
			return true;

		int ExitCode=0;

		const auto TypesMenu = VMenu2::create(msg(lng::MSelectAssocTitle), {}, ScrY - 4);
		TypesMenu->SetHelp(L"FileAssoc"sv);
		TypesMenu->SetMenuFlags(VMENU_WRAPMODE);
		TypesMenu->SetId(SelectAssocMenuId);
		for (auto& Item : MenuItems)
		{
			TypesMenu->AddItem(std::move(Item));
		}

		if (ActualCmdCount>1)
		{
			ExitCode=TypesMenu->Run();

			if (ExitCode<0)
				return true;
		}

		strCommand = *TypesMenu->GetComplexUserDataPtr<string>(ExitCode);
	}

	list_names ListNames;
	bool PreserveLFN = false;
	if (SubstFileName(strCommand, Context, &ListNames, &PreserveLFN) && !strCommand.empty())
	{
		SCOPED_ACTION(PreserveLongName)(ShortName, PreserveLFN);

		execute_info Info;
		Info.DisplayCommand = strCommand;
		Info.Command = strCommand;
		Info.WaitMode = AlwaysWaitFinish? execute_info::wait_mode::wait_finish : ListNames.any()? execute_info::wait_mode::wait_idle : execute_info::wait_mode::no_wait;
		Info.RunAs = RunAs;
		// We've already processed them!
		Info.UseAssociations = false;

		Launcher? Launcher(Info) : Global->CtrlObject->CmdLine()->ExecString(Info);

		if (AddToHistory && !(Global->Opt->ExcludeCmdHistory&EXCLUDECMDHISTORY_NOTFARASS) && !AlwaysWaitFinish) //AN
		{
			const auto curDir = Global->CtrlObject->CmdLine()->GetCurDir();
			Global->CtrlObject->CmdHistory->AddToHistory(strCommand, HR_DEFAULT, nullptr, {}, curDir);
		}
	}

	return true;
}


bool GetFiletypeOpenMode(int keyPressed, FILETYPE_MODE& mode, bool& shouldForceInternal)
{
	bool isModeFound = false;

	switch (keyPressed)
	{
	case KEY_F3:
	case KEY_NUMPAD5:
	case KEY_SHIFTNUMPAD5:
		mode = Global->Opt->ViOpt.UseExternalViewer? FILETYPE_ALTVIEW : FILETYPE_VIEW;
		shouldForceInternal = false;
		isModeFound = true;
		break;

	case KEY_CTRLSHIFTF3:
	case KEY_RCTRLSHIFTF3:
		mode = FILETYPE_VIEW;
		shouldForceInternal = true;
		isModeFound = true;
		break;

	case KEY_RALTF3:
	case KEY_ALTF3:
		mode = Global->Opt->ViOpt.UseExternalViewer? FILETYPE_VIEW : FILETYPE_ALTVIEW;
		shouldForceInternal = false;
		isModeFound = true;
		break;

	case KEY_F4:
		mode = Global->Opt->EdOpt.UseExternalEditor? FILETYPE_ALTEDIT: FILETYPE_EDIT;
		shouldForceInternal = false;
		isModeFound = true;
		break;

	case KEY_CTRLSHIFTF4:
	case KEY_RCTRLSHIFTF4:
		mode = FILETYPE_EDIT;
		shouldForceInternal = true;
		isModeFound = true;
		break;

	case KEY_ALTF4:
	case KEY_RALTF4:
		mode = Global->Opt->EdOpt.UseExternalEditor? FILETYPE_EDIT : FILETYPE_ALTEDIT;
		shouldForceInternal = false;
		isModeFound = true;
		break;

	case KEY_ENTER:
		mode = FILETYPE_EXEC;
		isModeFound = true;
		break;

	case KEY_CTRLPGDN:
	case KEY_RCTRLPGDN:
		mode = FILETYPE_ALTEXEC;
		isModeFound = true;
		break;

	default:
		// non-default key may be pressed, it's ok.
		break;
	}

	return isModeFound;
}

/*
  Используется для запуска внешнего редактора и вьювера
*/
void ProcessExternal(string_view const Command, string_view const Name, string_view const ShortName, bool const AlwaysWaitFinish)
{
	string strExecStr(Command);
	list_names ListNames;
	bool PreserveLFN = false;
	if (!SubstFileName(strExecStr, subst_context(Name, ShortName), &ListNames, &PreserveLFN) || strExecStr.empty())
		return;

	// If you want your history to be usable - use full paths yourself. We cannot reliably substitute them.
	Global->CtrlObject->ViewHistory->AddToHistory(strExecStr, AlwaysWaitFinish? HR_EXTERNAL_WAIT : HR_EXTERNAL);

	SCOPED_ACTION(PreserveLongName)(ShortName, PreserveLFN);

	execute_info Info;
	Info.DisplayCommand = strExecStr;
	Info.Command = strExecStr;
	Info.WaitMode = AlwaysWaitFinish? execute_info::wait_mode::wait_finish : ListNames.any()? execute_info::wait_mode::wait_idle : execute_info::wait_mode::no_wait;

	Global->CtrlObject->CmdLine()->ExecString(Info);
}

static auto FillFileTypesMenu(VMenu2* TypesMenu, int MenuPos)
{
	struct data_item
	{
		unsigned long long Id;
		string Mask;
		string Description;
	};

	std::vector<data_item> Data;

	for(auto& [Id, Mask]: ConfigProvider().AssocConfig()->MasksEnumerator())
	{
		data_item Item{ Id, std::move(Mask) };
		ConfigProvider().AssocConfig()->GetDescription(Item.Id, Item.Description);
		Data.emplace_back(std::move(Item));
	}

	const auto MaxElement = std::max_element(ALL_CONST_RANGE(Data), [](const auto& a, const auto &b) { return a.Description.size() < b.Description.size(); });

	TypesMenu->clear();

	for (const auto& i: Data)
	{
		const auto AddLen = i.Description.size() - HiStrlen(i.Description);
		MenuItemEx TypesMenuItem(concat(fit_to_left(i.Description, MaxElement->Description.size() + AddLen), L' ', BoxSymbols[BS_V1], L' ', i.Mask));
		TypesMenuItem.ComplexUserData = i.Id;
		TypesMenu->AddItem(TypesMenuItem);
	}

	TypesMenu->SetSelectPos(MenuPos);

	return static_cast<int>(Data.size());
}

enum EDITTYPERECORD
{
	ETR_DOUBLEBOX,
	ETR_TEXT_MASKS,
	ETR_EDIT_MASKS,
	ETR_TEXT_DESCR,
	ETR_EDIT_DESCR,
	ETR_SEPARATOR1,
	ETR_COMBO_EXEC,
	ETR_EDIT_EXEC,
	ETR_COMBO_ALTEXEC,
	ETR_EDIT_ALTEXEC,
	ETR_COMBO_VIEW,
	ETR_EDIT_VIEW,
	ETR_COMBO_ALTVIEW,
	ETR_EDIT_ALTVIEW,
	ETR_COMBO_EDIT,
	ETR_EDIT_EDIT,
	ETR_COMBO_ALTEDIT,
	ETR_EDIT_ALTEDIT,
	ETR_SEPARATOR2,
	ETR_BUTTON_OK,
	ETR_BUTTON_CANCEL,
};

static intptr_t EditTypeRecordDlgProc(Dialog* Dlg,intptr_t Msg,intptr_t Param1,void* Param2)
{
	switch (Msg)
	{
		case DN_BTNCLICK:

			switch (Param1)
			{
				case ETR_COMBO_EXEC:
				case ETR_COMBO_ALTEXEC:
				case ETR_COMBO_VIEW:
				case ETR_COMBO_ALTVIEW:
				case ETR_COMBO_EDIT:
				case ETR_COMBO_ALTEDIT:
					Dlg->SendMessage(DM_ENABLE,Param1+1,ToPtr(reinterpret_cast<intptr_t>(Param2)==BSTATE_CHECKED));
					break;
				default:
					break;
			}

			break;
		case DN_CLOSE:

			if (Param1==ETR_BUTTON_OK)
			{
				return filemasks().Set(reinterpret_cast<const wchar_t*>(Dlg->SendMessage(DM_GETCONSTTEXTPTR, ETR_EDIT_MASKS, nullptr)));
			}
			break;

		default:
			break;
	}

	return Dlg->DefProc(Msg,Param1,Param2);
}

static bool EditTypeRecord(unsigned long long EditPos,bool NewRec)
{
	const int DlgX=76,DlgY=23;
	FarDialogItem const EditDlgData[]
	{
		{DI_DOUBLEBOX,3, 1,DlgX-4,DlgY-2,0,nullptr,nullptr,0,msg(lng::MFileAssocTitle).c_str()},
		{DI_TEXT,     5, 2, 0, 2,0,nullptr,nullptr,0,msg(lng::MFileAssocMasks).c_str()},
		{DI_EDIT,     5, 3,DlgX-6, 3,0,L"Masks",nullptr,DIF_FOCUS|DIF_HISTORY,L""},
		{DI_TEXT,     5, 4, 0, 4,0,nullptr,nullptr,0,msg(lng::MFileAssocDescr).c_str()},
		{DI_EDIT,     5, 5,DlgX-6, 5,0,nullptr,nullptr,0,L""},
		{DI_TEXT,     -1, 6, 0, 6,0,nullptr,nullptr,DIF_SEPARATOR,L""},
		{DI_CHECKBOX, 5, 7, 0, 7,1,nullptr,nullptr,0,msg(lng::MFileAssocExec).c_str()},
		{DI_EDIT,     9, 8,DlgX-6, 8,0,nullptr,nullptr,DIF_EDITPATH|DIF_EDITPATHEXEC,L""},
		{DI_CHECKBOX, 5, 9, 0, 9,1,nullptr,nullptr,0,msg(lng::MFileAssocAltExec).c_str()},
		{DI_EDIT,     9,10,DlgX-6,10,0,nullptr,nullptr,DIF_EDITPATH|DIF_EDITPATHEXEC,L""},
		{DI_CHECKBOX, 5,11, 0,11,1,nullptr,nullptr,0,msg(lng::MFileAssocView).c_str()},
		{DI_EDIT,     9,12,DlgX-6,12,0,nullptr,nullptr,DIF_EDITPATH|DIF_EDITPATHEXEC,L""},
		{DI_CHECKBOX, 5,13, 0,13,1,nullptr,nullptr,0,msg(lng::MFileAssocAltView).c_str()},
		{DI_EDIT,     9,14,DlgX-6,14,0,nullptr,nullptr,DIF_EDITPATH|DIF_EDITPATHEXEC,L""},
		{DI_CHECKBOX, 5,15, 0,15,1,nullptr,nullptr,0,msg(lng::MFileAssocEdit).c_str()},
		{DI_EDIT,     9,16,DlgX-6,16,0,nullptr,nullptr,DIF_EDITPATH|DIF_EDITPATHEXEC,L""},
		{DI_CHECKBOX, 5,17, 0,17,1,nullptr,nullptr,0,msg(lng::MFileAssocAltEdit).c_str()},
		{DI_EDIT,     9,18,DlgX-6,18,0,nullptr,nullptr,DIF_EDITPATH|DIF_EDITPATHEXEC,L""},
		{DI_TEXT,     -1,DlgY-4, 0,DlgY-4,0,nullptr,nullptr,DIF_SEPARATOR,L""},
		{DI_BUTTON,   0,DlgY-3, 0,DlgY-3,0,nullptr,nullptr,DIF_DEFAULTBUTTON|DIF_CENTERGROUP,msg(lng::MOk).c_str()},
		{DI_BUTTON,   0,DlgY-3, 0,DlgY-3,0,nullptr,nullptr,DIF_CENTERGROUP,msg(lng::MCancel).c_str()},
	};
	auto EditDlg = MakeDialogItemsEx(EditDlgData);

	if (!NewRec)
	{
		ConfigProvider().AssocConfig()->GetMask(EditPos,EditDlg[ETR_EDIT_MASKS].strData);
		ConfigProvider().AssocConfig()->GetDescription(EditPos,EditDlg[ETR_EDIT_DESCR].strData);
		for (int i=FILETYPE_EXEC,Item=ETR_EDIT_EXEC; i<=FILETYPE_ALTEDIT; i++,Item+=2)
		{
			bool on=false;
			if (!ConfigProvider().AssocConfig()->GetCommand(EditPos,i,EditDlg[Item].strData,&on) || !on)
			{
				EditDlg[Item-1].Selected = BSTATE_UNCHECKED;
				EditDlg[Item].Flags |= DIF_DISABLE;
			}
			else
			{
				EditDlg[Item-1].Selected = BSTATE_CHECKED;
			}
		}
	}

	const auto Dlg = Dialog::create(EditDlg, EditTypeRecordDlgProc);
	Dlg->SetHelp(L"FileAssocModify"sv);
	Dlg->SetId(FileAssocModifyId);
	Dlg->SetPosition({ -1, -1, DlgX, DlgY });
	Dlg->Process();

	if (Dlg->GetExitCode()==ETR_BUTTON_OK)
	{
		if (NewRec)
		{
			EditPos = ConfigProvider().AssocConfig()->AddType(EditPos,EditDlg[ETR_EDIT_MASKS].strData,EditDlg[ETR_EDIT_DESCR].strData);
		}
		else
		{
			ConfigProvider().AssocConfig()->UpdateType(EditPos,EditDlg[ETR_EDIT_MASKS].strData,EditDlg[ETR_EDIT_DESCR].strData);
		}

		for (int i=FILETYPE_EXEC,Item=ETR_EDIT_EXEC; i<=FILETYPE_ALTEDIT; i++,Item+=2)
		{
			ConfigProvider().AssocConfig()->SetCommand(EditPos,i,EditDlg[Item].strData,EditDlg[Item-1].Selected==BSTATE_CHECKED);
		}

		return true;
	}

	return false;
}

static bool DeleteTypeRecord(unsigned long long DeletePos)
{
	string strMask;
	ConfigProvider().AssocConfig()->GetMask(DeletePos,strMask);
	inplace::quote_unconditional(strMask);

	if (Message(MSG_WARNING,
		msg(lng::MAssocTitle),
		{
			msg(lng::MAskDelAssoc),
			strMask
		},
		{ lng::MDelete, lng::MCancel }) == Message::first_button)
	{
		ConfigProvider().AssocConfig()->DelType(DeletePos);
		return true;
	}

	return false;
}

void EditFileTypes()
{
	SCOPED_ACTION(auto)(ConfigProvider().AssocConfig()->ScopedTransaction());

	int MenuPos=0;
	const auto TypesMenu = VMenu2::create(msg(lng::MAssocTitle), {}, ScrY - 4);
	TypesMenu->SetHelp(L"FileAssoc"sv);
	TypesMenu->SetMenuFlags(VMENU_WRAPMODE);
	TypesMenu->SetBottomTitle(msg(lng::MAssocBottom));
	TypesMenu->SetId(FileAssocMenuId);
	
	bool Changed = false;
	for (;;)
	{
		int NumLine = FillFileTypesMenu(TypesMenu.get(), MenuPos);
		int ExitCode=TypesMenu->Run([&](const Manager::Key& RawKey)
		{
			const auto Key=RawKey();
			MenuPos=TypesMenu->GetSelectPos();

			int KeyProcessed = 1;

			switch (Key)
			{
				case KEY_NUMDEL:
				case KEY_DEL:
					if (MenuPos<NumLine)
					{
						if (const auto IdPtr = TypesMenu->GetComplexUserDataPtr<unsigned long long>(MenuPos))
						{
							Changed = DeleteTypeRecord(*IdPtr);
						}
					}
					break;

				case KEY_NUMPAD0:
				case KEY_INS:
					if (MenuPos - 1 >= 0)
					{
						if (const auto IdPtr = TypesMenu->GetComplexUserDataPtr<unsigned long long>(MenuPos - 1))
						{
							Changed = EditTypeRecord(*IdPtr, true);
						}
					}
					else
					{
						Changed = EditTypeRecord(0, true);
					}
					break;

				case KEY_NUMENTER:
				case KEY_ENTER:
				case KEY_F4:
					if (MenuPos<NumLine)
					{
						if (const auto IdPtr = TypesMenu->GetComplexUserDataPtr<unsigned long long>(MenuPos))
						{
							Changed = EditTypeRecord(*IdPtr, false);
						}
					}
					break;

				case KEY_CTRLUP:
				case KEY_RCTRLUP:
				case KEY_CTRLDOWN:
				case KEY_RCTRLDOWN:
				{
					if (!((Key==KEY_CTRLUP || Key==KEY_RCTRLUP) && !MenuPos) &&
						!((Key == KEY_CTRLDOWN || Key == KEY_RCTRLDOWN) && MenuPos == static_cast<int>(TypesMenu->size() - 1)))
					{
						int NewMenuPos=MenuPos+((Key==KEY_CTRLUP || Key==KEY_RCTRLUP)?-1:+1);
						if (const auto IdPtr = TypesMenu->GetComplexUserDataPtr<unsigned long long>(MenuPos))
						{
							if (const auto IdPtr2 = TypesMenu->GetComplexUserDataPtr<unsigned long long>(NewMenuPos))
							{
								if (ConfigProvider().AssocConfig()->SwapPositions(*IdPtr, *IdPtr2))
								{
									MenuPos = NewMenuPos;
									Changed = true;
								}
							}
						}
					}
				}
				break;

				default:
					KeyProcessed = 0;
			}
			if (Changed)
			{
				Changed = false;
				NumLine = FillFileTypesMenu(TypesMenu.get(), MenuPos);
			}
			return KeyProcessed;
		});

		if (ExitCode!=-1)
		{
			MenuPos=ExitCode;
			TypesMenu->Key(KEY_F4);
			continue;
		}

		break;
	}
}
