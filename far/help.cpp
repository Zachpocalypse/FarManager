/*
help.cpp

������

*/

/* Revision: 1.04 25.08.2000 $ */

/*
Modify:
  25.08.2000 SVS
    + CtrlAltShift - ������/�������� ������...
    + URL ��⨢��� - �� ���� ⠪ ���� :-)))
  13.07.2000 SVS
    ! ������� ���४樨 �� �ᯮ�짮����� new/delete/realloc
  28.06.2000
    - NT Console resize
      adding SetScreenPosition method
  26.06.2000 IS
    - ��� � 奫��� �� f1, shift+f2, end
      (�襭�� �।����� IG)
  25.06.2000 SVS
    ! �����⮢�� Master Copy
    ! �뤥����� � ����⢥ ᠬ����⥫쭮�� �����
*/

#include "headers.hpp"
#pragma hdrstop

/* $ 30.06.2000 IS
   �⠭����� ���������
*/
#include "internalheaders.hpp"
/* IS $ */

#define MAX_HELP_STRING_LENGTH 300

static int FullScreenHelp=0;
static SaveScreen *TopScreen=NULL;

static char *PluginContents="__PluginContents__";
static char *HelpOnHelpTopic="Help";

/* $ 25.08.2000 SVS
   ����� URL-��뫮�... ;-)
   �� ���� ⠪ ����... ���?
*/
static BOOL RunURL(char *Protocol, char *URLPath)
{
  BOOL EditCode=FALSE;
  if(Protocol && *Protocol && URLPath && *URLPath)
  {
    char *Buf=(char*)malloc(2048);
    if(Buf)
    {
      HKEY hKey;
      DWORD Disposition, DataSize=250;
      strcpy(Buf,Protocol);
      strcat(Buf,"\\shell\\open\\command");
      if(RegOpenKeyEx(HKEY_CLASSES_ROOT,Buf,0,KEY_READ,&hKey) == ERROR_SUCCESS)
      {
        Disposition=RegQueryValueEx(hKey,"",0,&Disposition,(LPBYTE)Buf,&DataSize);
        RegCloseKey(hKey);
        if(Disposition == ERROR_SUCCESS)
        {
          char *pp=strrchr(Buf,'%');

          if(pp)
            *pp='\0';
          else
            strcat(Buf," ");
          STARTUPINFO si={0};
          PROCESS_INFORMATION pi={0};
          si.cb=sizeof(si);
          strcat(Buf,URLPath);
          EditCode=CreateProcess(NULL,Buf,NULL,NULL,
                                       TRUE,0,NULL,NULL,&si,&pi);
        }
      }
      free(Buf);
    }
  }
  return EditCode;
}
/* SVS $ */

Help::Help(char *Topic)
{
  if (PrevMacroMode!=MACRO_HELP)
  {
    PrevMacroMode=CtrlObject->Macro.GetMode();
    CtrlObject->Macro.SetMode(MACRO_HELP);
  }
  TopLevel=TRUE;
  TopScreen=new SaveScreen;
  HelpData=NULL;
  strcpy(HelpTopic,Topic);
  *HelpPath=0;
  if (FullScreenHelp)
    SetPosition(0,0,ScrX,ScrY);
  else
    SetPosition(4,2,ScrX-4,ScrY-2);
  ReadHelp();
  if (HelpData!=NULL)
    Process();
  else
    Message(MSG_WARNING,1,MSG(MHelpTitle),MSG(MHelpTopicNotFound),MSG(MOk));
}


Help::Help(char *Topic,int &ShowPrev,int PrevFullScreen)
{
  if (PrevMacroMode!=MACRO_HELP)
  {
    PrevMacroMode=CtrlObject->Macro.GetMode();
    CtrlObject->Macro.SetMode(MACRO_HELP);
  }
  TopLevel=FALSE;
  HelpData=NULL;
  Help::PrevFullScreen=PrevFullScreen;
  strcpy(HelpTopic,Topic);
  *HelpPath=0;
  if (FullScreenHelp)
    SetPosition(0,0,ScrX,ScrY);
  else
    SetPosition(4,2,ScrX-4,ScrY-2);
  ReadHelp();
  if (HelpData!=NULL)
    Process();
  else
    Message(MSG_WARNING,1,MSG(MHelpTitle),MSG(MHelpTopicNotFound),MSG(MOk));
  ShowPrev=Help::ShowPrev;
}


Help::~Help()
{
  CtrlObject->Macro.SetMode(PrevMacroMode);
  SetRestoreScreenMode(FALSE);
  /* $ 13.07.2000
    ��� �뤥����� ����� �ᯮ�짮������ �㭪�� realloc
  */
  free(HelpData);
  /* SVS $ */
  if (TopLevel)
  {
    delete TopScreen;
    TopScreen=NULL;
  }
  if (TopScreen!=NULL && (TopLevel || PrevFullScreen!=FullScreenHelp))
    TopScreen->RestoreArea();
}


void Help::Hide()
{
  ScreenObject::Hide();
  if (TopScreen!=NULL)
    TopScreen->RestoreArea();
}


void Help::ReadHelp()
{
  char FileName[NM],ReadStr[MAX_HELP_STRING_LENGTH];
  char SplitLine[2*MAX_HELP_STRING_LENGTH];
  int Formatting=TRUE,RepeatLastLine;
  const int MaxLength=X2-X1-1;

  ShowPrev=0;
  DisableOut=0;

  char Path[NM],*TopicPtr;
  if (*HelpTopic=='#')
  {
    strcpy(Path,HelpTopic+1);
    if ((TopicPtr=strchr(Path,'#'))==NULL)
      return;
    strcpy(HelpTopic,TopicPtr+1);
    *TopicPtr=0;
    strcpy(HelpPath,Path);
  }
  else
    strcpy(Path,*HelpPath ? HelpPath:FarPath);

  if (strcmp(HelpTopic,PluginContents)==0)
  {
    ReadPluginsHelp();
    return;
  }

  FILE *HelpFile=Language::OpenLangFile(Path,HelpFileMask,Opt.HelpLanguage,FileName);

  if (HelpFile==NULL)
  {
    Message(MSG_WARNING|MSG_ERRORTYPE,1,MSG(MHelpTitle),MSG(MCannotOpenHelp),FileName,MSG(MOk));
    return;
  }
  *SplitLine=0;
  CurX=CurY=0;
  if (HelpData!=NULL)
    /* $ 13.07.2000
      ��� �뤥����� ����� �ᯮ�짮������ �㭪�� realloc
    */
    free(HelpData);
    /* SVS $ */
  HelpData=NULL;
  StrCount=0;
  FixCount=0;
  TopStr=0;
  TopicFound=0;
  RepeatLastLine=FALSE;
  while (TRUE)
  {
    if (!RepeatLastLine && fgets(ReadStr,sizeof(ReadStr),HelpFile)==NULL)
    {
      if (StringLen(SplitLine)<MaxLength)
      {
        if (*SplitLine)
          AddLine(SplitLine);
      }
      else
      {
        *ReadStr=0;
        RepeatLastLine=TRUE;
        continue;
      }
      break;
    }
    RepeatLastLine=FALSE;

    RemoveTrailingSpaces(ReadStr);

    if (TopicFound)
      HighlightsCorrection(ReadStr);

    if (*ReadStr=='@')
    {
      if (TopicFound)
      {
        if (strcmp(ReadStr,"@+")==0)
        {
          Formatting=TRUE;
          continue;
        }
        if (strcmp(ReadStr,"@-")==0)
        {
          Formatting=FALSE;
          continue;
        }
        if (*SplitLine)
          AddLine(SplitLine);
        break;
      }
      else
        if (LocalStricmp(ReadStr+1,HelpTopic)==0)
          TopicFound=1;
    }
    else
      if (TopicFound)
      {
        if (*ReadStr=='$')
        {
          AddLine(ReadStr+1);
          FixCount++;
        }
        else
        {
          if (*ReadStr==0 || !Formatting)
            if (*SplitLine)
              if (StringLen(SplitLine)<MaxLength)
              {
                AddLine(SplitLine);
                *SplitLine=0;
                if (StringLen(ReadStr)<MaxLength)
                {
                  AddLine(ReadStr);
                  continue;
                }
              }
              else
                RepeatLastLine=TRUE;
            else
              if (StringLen(ReadStr)<MaxLength)
              {
                AddLine(ReadStr);
                continue;
              }
          if (isspace(*ReadStr) && Formatting)
            if (StringLen(SplitLine)<MaxLength)
            {
              if (*SplitLine)
                AddLine(SplitLine);
              strcpy(SplitLine,ReadStr);
              *ReadStr=0;
              continue;
            }
            else
              RepeatLastLine=TRUE;
          if (!RepeatLastLine)
          {
            if (*SplitLine)
              strcat(SplitLine," ");
            strcat(SplitLine,ReadStr);
          }
          if (StringLen(SplitLine)<MaxLength)
            continue;
          int Splitted=0;
          for (int I=strlen(SplitLine)-1;I>0;I--)
          {
            if (I>0 && SplitLine[I]=='~' && SplitLine[I-1]=='~')
            {
              I--;
              continue;
            }
            if (I>0 && SplitLine[I]=='~' && SplitLine[I-1]!='~')
            {
              do {
                I--;
              } while (I>0 && SplitLine[I]!='~');
              continue;
            }
            if (SplitLine[I]==' ')
            {
              SplitLine[I]=0;
              if (StringLen(SplitLine)<MaxLength)
              {
                AddLine(SplitLine);
                char CopyLine[2*MAX_HELP_STRING_LENGTH];
                strcpy(CopyLine,SplitLine+I+1);
                *SplitLine=' ';
                strcpy(SplitLine+1,CopyLine);
                HighlightsCorrection(SplitLine);
                Splitted=TRUE;
                break;
              }
              else
                SplitLine[I]=' ';
            }
          }
          if (!Splitted)
          {
            AddLine(SplitLine);
            *SplitLine=0;
          }
        }
      }
  }
  fclose(HelpFile);
  FixSize=FixCount+(FixCount!=0);
}


void Help::AddLine(char *Line)
{
  char *NewHelpData=(char *)realloc(HelpData,(StrCount+1)*MAX_HELP_STRING_LENGTH);
  if (NewHelpData==NULL)
    return;
  HelpData=NewHelpData;
  strncpy(HelpData+StrCount*MAX_HELP_STRING_LENGTH,Line,MAX_HELP_STRING_LENGTH);
  StrCount++;
}


void Help::HighlightsCorrection(char *Str)
{
  int I,Count;
  for (I=0,Count=0;Str[I]!=0;I++)
    if (Str[I]=='#')
      Count++;
  if ((Count & 1) && *Str!='$')
  {
    char CopyLine[MAX_HELP_STRING_LENGTH];
    strcpy(CopyLine,Str);
    *Str='#';
    strcpy(Str+1,CopyLine);
  }
}


void Help::DisplayObject()
{
  if (!TopicFound)
  {
    Message(MSG_WARNING,1,MSG(MHelpTitle),MSG(MHelpTopicNotFound),MSG(MOk));
    ProcessKey(KEY_ALTF1);
    return;
  }
  SetCursorType(0,10);
  MoveToReference(1,1);
  FastShow();
}


void Help::FastShow()
{
  int I;
  if (!DisableOut)
  {
    SetScreen(X1,Y1,X2,Y2,' ',COL_HELPTEXT);
    Box(X1,Y1,X2,Y2,COL_HELPBOX,DOUBLE_BOX);
    SetColor(COL_HELPBOXTITLE);
    GotoXY(X1+(X2-X1+1-strlen(MSG(MHelpTitle))-2)/2,Y1);
    mprintf(" %s ",MSG(MHelpTitle));
  }
  CorrectPosition();
  *SelTopic=0;
  for (I=0;I<Y2-Y1-1;I++)
  {
    int StrPos;
    if (I<FixCount)
      StrPos=I;
    else
      if (I==FixCount && FixCount>0)
      {
        if (!DisableOut)
        {
          GotoXY(X1,Y1+I+1);
          SetColor(COL_HELPBOX);
          ShowSeparator(X2-X1+1);
        }
        continue;
      }
      else
      {
        StrPos=I+TopStr;
        if (FixCount>0)
          StrPos--;
      }
    if (StrPos<StrCount)
    {
      char *OutStr=HelpData+StrPos*MAX_HELP_STRING_LENGTH;
      if (*OutStr=='^')
      {
        GotoXY(X1+(X2-X1+1-StringLen(OutStr))/2,Y1+I+1);
        OutStr++;
      }
      else
        GotoXY(X1+1,Y1+I+1);
      OutString(OutStr);
    }
  }

  const int ScrollLength=Y2-Y1-FixSize-1;
  if (!DisableOut && StrCount-FixCount > ScrollLength)
  {
    int Scrolled=StrCount-FixCount-ScrollLength;
    SetColor(COL_HELPSCROLLBAR);
    ScrollBar(X2,Y1+FixSize+1,ScrollLength,TopStr,Scrolled);
  }
}


void Help::OutString(char *Str)
{
  char OutStr[512],*StartTopic=NULL;
  int OutPos=0,Highlight=0,Topic=0;
  while (OutPos<sizeof(OutStr)-10)
  {
    if (Str[0]=='~' && Str[1]=='~' || Str[0]=='#' && Str[1]=='#' ||
        Str[0]=='@' && Str[1]=='@')
    {
      OutStr[OutPos++]=*Str;
      Str+=2;
      continue;
    }
    if (*Str=='~' || *Str=='#' || *Str==0)
    {
      OutStr[OutPos]=0;
      if (Topic)
      {
        int RealCurX,RealCurY;
        RealCurX=X1+CurX+1;
        RealCurY=Y1+CurY+FixSize+1;
        if (WhereY()==RealCurY && RealCurX>=WhereX() &&
                RealCurX<WhereX()+(Str-StartTopic)-1)
        {
          SetColor(COL_HELPSELECTEDTOPIC);
          if (Str[1]=='@')
          {
            strncpy(SelTopic,Str+2,sizeof(SelTopic));
            char *EndPtr=strchr(SelTopic,'@');
            /* $ 25.08.2000 SVS
               ��⥬, �� ����� ���� ⠪�� ��ਠ��: @@ ��� \@
               ��� ��ਠ�� ⮫쪮 ��� URL!
            */
            if (EndPtr!=NULL)
            {
              if(*(EndPtr-1) == '\\')
              {
                memmove(EndPtr-1,EndPtr,strlen(EndPtr)+1);
              }
              else if(*(EndPtr+1) == '@')
              {
                memmove(EndPtr,EndPtr+1,strlen(EndPtr)+1);
                EndPtr++;
              }
              EndPtr=strchr(EndPtr,'@');
              if (EndPtr!=NULL)
                *EndPtr=0;
            }
            /* SVS $ */
          }
        }
        else
          SetColor(COL_HELPTOPIC);
      }
      else
        if (Highlight)
          SetColor(COL_HELPHIGHLIGHTTEXT);
        else
          SetColor(COL_HELPTEXT);
      if (DisableOut)
        GotoXY(WhereX()+strlen(OutStr),WhereY());
      else
        Text(OutStr);
      OutPos=0;
    }

    if (*Str==0)
      break;

    if (*Str=='~')
    {
      if (!Topic)
        StartTopic=Str;
      Topic=!Topic;
      Str++;
      continue;
    }
    if (*Str=='@')
    {
      /* $ 25.08.2000 SVS
         ��⥬, �� ����� ���� ⠪�� ��ਠ��: @@ ��� \@
         ��� ��ਠ�� ⮫쪮 ��� URL!
      */
      while (*Str)
        if (*(++Str)=='@')
         if (*(Str-1)!='@' && *(Str-1)!='\\')
           break;
      /* SVS $ */
      Str++;
      continue;
    }
    if (*Str=='#')
    {
      Highlight=!Highlight;
      Str++;
      continue;
    }
    OutStr[OutPos++]=*(Str++);
  }
  if (!DisableOut && WhereX()<X2)
  {
    SetColor(COL_HELPTEXT);
    mprintf("%*s",X2-WhereX(),"");
  }
}


int Help::StringLen(char *Str)
{
  int Length=0;
  while (*Str)
  {
    if (Str[0]=='~' && Str[1]=='~' || Str[0]=='#' && Str[1]=='#' ||
        Str[0]=='@' && Str[1]=='@')
    {
      Length++;
      Str+=2;
      continue;
    }
    if (*Str=='@')
    {
      /* $ 25.08.2000 SVS
         ��⥬, �� ����� ���� ⠪�� ��ਠ��: @@ ��� \@
         ��� ��ਠ�� ⮫쪮 ��� URL!
      */
      while (*Str)
        if (*(++Str)=='@')
          if (*(Str-1)!='@' && *(Str-1)!='\\')
            break;
      /* SVS $ */
      Str++;
      continue;
    }
    if (*Str!='#' && *Str!='~')
      Length++;
    Str++;
  }
  return(Length);
}


void Help::CorrectPosition()
{
  if (CurX>X2-X1-2)
    CurX=X2-X1-2;
  if (CurX<0)
    CurX=0;
  if (CurY>Y2-Y1-2-FixSize)
  {
    TopStr+=CurY-(Y2-Y1-2-FixSize);
    CurY=Y2-Y1-2-FixSize;
  }
  if (CurY<0)
  {
    TopStr+=CurY;
    CurY=0;
  }
  if (TopStr>StrCount-FixCount-(Y2-Y1-1-FixSize))
    TopStr=StrCount-FixCount-(Y2-Y1-1-FixSize);
  if (TopStr<0)
    TopStr=0;
}


int Help::ProcessKey(int Key)
{
  if (*SelTopic==0)
    CurX=CurY=0;
  switch(Key)
  {
    case KEY_NONE:
    case KEY_IDLE:
      break;
    /* $ 25.08.2000 SVS
       + CtrlAltShift - ������/�������� ������...
       // "���� ������ �� �, �� ��� ��������..."
    */
    case KEY_CTRLALTSHIFTPRESS:
    {
       Hide();
       WaitKey(KEY_CTRLALTSHIFTRELEASE);
       Show();
       return(TRUE);
    }
    /* SVS $ */
    case KEY_F1:
      strcpy(SelTopic,HelpOnHelpTopic);
      ProcessKey(KEY_ENTER);
      return(TRUE);
    case KEY_F5:
      Hide();
      FullScreenHelp=!FullScreenHelp;
      if (FullScreenHelp)
        SetPosition(0,0,ScrX,ScrY);
      else
        SetPosition(4,2,ScrX-4,ScrY-2);
      ReadHelp();
      Show();
      return(TRUE);
    case KEY_ESC:
    case KEY_F10:
      ShowPrev=FALSE;
      EndLoop=TRUE;
      return(TRUE);
    case KEY_ALTF1:
    case KEY_BS:
      ShowPrev=TRUE;
      EndLoop=TRUE;
      return(TRUE);
    case KEY_HOME:
    case KEY_CTRLHOME:
    case KEY_CTRLPGUP:
      CurX=CurY=0;
      TopStr=0;
      FastShow();
      if (*SelTopic==0)
        MoveToReference(1,1);
      return(TRUE);
    case KEY_END:
    case KEY_CTRLEND:
    case KEY_CTRLPGDN:
      CurX=CurY=0;
      TopStr=StrCount;
      FastShow();
      if (*SelTopic==0)
      {
        CurX=0;
        CurY=Y2-Y1-2-FixSize;
        MoveToReference(0,1);
      }
      return(TRUE);
    case KEY_UP:
      if (TopStr>0)
      {
        TopStr--;
        if (CurY<Y2-Y1-2-FixSize)
        {
          CurX=X2-X1-2;
          CurY++;
        }
        FastShow();
        if (*SelTopic==0)
          MoveToReference(0,1);
      }
      else
        ProcessKey(KEY_SHIFTTAB);
      return(TRUE);
    case KEY_DOWN:
      if (TopStr<StrCount-FixCount-(Y2-Y1-1-FixSize))
      {
        TopStr++;
        if (CurY>0)
          CurY--;
        CurX=0;
        FastShow();
        if (*SelTopic==0)
          MoveToReference(1,1);
      }
      else
        ProcessKey(KEY_TAB);
      return(TRUE);
    case KEY_PGUP:
      CurX=CurY=0;
      TopStr-=Y2-Y1-2-FixSize;
      FastShow();
      if (*SelTopic==0)
      {
        CurX=CurY=0;
        MoveToReference(1,1);
      }
      return(TRUE);
    case KEY_PGDN:
      {
        int PrevTopStr=TopStr;
        TopStr+=Y2-Y1-2-FixSize;
        FastShow();
        if (TopStr==PrevTopStr)
        {
          ProcessKey(KEY_CTRLPGDN);
          return(TRUE);
        }
        else
          CurX=CurY=0;
        MoveToReference(1,1);
      }
      return(TRUE);
    case KEY_ENTER:
      if (*SelTopic && LocalStricmp(HelpTopic,SelTopic)!=0)
      {
        int KeepHelp;
        int CurFullScreen=FullScreenHelp;
        {
          char NewTopic[512];
          /* $ 25.08.2000 SVS
             URL ��⨢��� - �� ���� ⠪ ���� :-)))
          */
          {
            strcpy(NewTopic,SelTopic);
            char *p=strchr(NewTopic,':');
            if(p && NewTopic[0] != ':') // ����୮� ���ࠧ㬥������ URL
            {
              *p=0;
              if(RunURL(NewTopic,SelTopic))
                return(TRUE);
            }
            // � ��� ⥯��� ���஡㥬...
          }
          /* SVS $ */
          if (*HelpPath && *SelTopic!='#' && strcmp(SelTopic,HelpOnHelpTopic)!=0)
          {
            if (*SelTopic==':')
              strcpy(NewTopic,SelTopic+1);
            else
              sprintf(NewTopic,"#%s#%s",HelpPath,SelTopic);
          }
          else
            strcpy(NewTopic,SelTopic);

          Help NewHelp(NewTopic,KeepHelp,FullScreenHelp);
        }
        if (!KeepHelp)
          EndLoop=TRUE;
        else
        {
          FastShow();
          if (CurFullScreen!=FullScreenHelp)
          {
            FullScreenHelp=!FullScreenHelp;
            ProcessKey(KEY_F5);
          }
        }
      }
      return(TRUE);
    case KEY_SHIFTF1:
      strcpy(SelTopic,"Contents");
      ProcessKey(KEY_ENTER);
      return(TRUE);
    case KEY_SHIFTF2:
      strcpy(SelTopic,PluginContents);
      ProcessKey(KEY_ENTER);
      return(TRUE);
    case KEY_RIGHT:
    case KEY_TAB:
      MoveToReference(1,0);
      return(TRUE);
    case KEY_LEFT:
    case KEY_SHIFTTAB:
      MoveToReference(0,0);
      return(TRUE);
  }
  return(FALSE);
}


int Help::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
  if (MouseEvent->dwEventFlags==MOUSE_MOVED && (MouseEvent->dwButtonState & 3)==0)
    return(FALSE);
  if (MouseEvent->dwMousePosition.X<X1 || MouseEvent->dwMousePosition.X>X2 ||
      MouseEvent->dwMousePosition.Y<Y1 || MouseEvent->dwMousePosition.Y>Y2)
  {
    ProcessKey(KEY_ESC);
    return(TRUE);
  }
  if (MouseX==X2 && (MouseEvent->dwButtonState & 1))
  {
    int ScrollY=Y1+FixSize+1;
    int Height=Y2-Y1-FixSize-1;
    if (MouseY==ScrollY)
    {
      while (IsMouseButtonPressed())
        ProcessKey(KEY_UP);
      return(TRUE);
    }
    if (MouseY==ScrollY+Height-1)
    {
      while (IsMouseButtonPressed())
        ProcessKey(KEY_DOWN);
      return(TRUE);
    }
  }
  if (MouseEvent->dwMousePosition.Y<Y1+1+FixSize)
  {
    while (IsMouseButtonPressed() && MouseY<Y1+1+FixSize)
      ProcessKey(KEY_UP);
    return(TRUE);
  }
  if (MouseEvent->dwMousePosition.Y>=Y2)
  {
    while (IsMouseButtonPressed() && MouseY>=Y2)
      ProcessKey(KEY_DOWN);
    return(TRUE);
  }
  CurX=MouseEvent->dwMousePosition.X-X1-1;
  CurY=MouseEvent->dwMousePosition.Y-Y1-1-FixSize;
  FastShow();
  if ((MouseEvent->dwButtonState & 3)==0 && *SelTopic)
    ProcessKey(KEY_ENTER);
  return(TRUE);
}


int Help::IsReferencePresent()
{
  CorrectPosition();
  int StrPos=FixCount+TopStr+CurY;
  char *OutStr=HelpData+StrPos*MAX_HELP_STRING_LENGTH;
  return (strchr(OutStr,'@')!=NULL && strchr(OutStr,'~')!=NULL);
}


void Help::MoveToReference(int Forward,int CurScreen)
{
  int StartSelection=*SelTopic;
  int SaveCurX=CurX,SaveCurY=CurY,SaveTopStr=TopStr;
  *SelTopic=0;
  DisableOut=TRUE;
  while (*SelTopic==0)
  {
    if (Forward)
    {
      if (CurX==0 && !IsReferencePresent())
        CurX=X2-X1-2;
      if (++CurX >= X2-X1-2)
      {
        StartSelection=0;
        CurX=0;
        CurY++;
        if (TopStr+CurY>=StrCount-FixCount ||
            CurScreen && CurY>Y2-Y1-2-FixSize)
          break;
      }
    }
    else
    {
      if (CurX==X2-X1-2 && !IsReferencePresent())
        CurX=0;
      if (--CurX < 0)
      {
        StartSelection=0;
        CurX=X2-X1-2;
        CurY--;
        if (TopStr+CurY<0 || CurScreen && CurY<0)
          break;
      }
    }

    FastShow();
    if (*SelTopic==0)
      StartSelection=0;
    else
      if (StartSelection)
        *SelTopic=0;
  }
  DisableOut=FALSE;
  if (*SelTopic==0)
  {
    CurX=SaveCurX;
    CurY=SaveCurY;
    TopStr=SaveTopStr;
  }
  FastShow();
}


int Help::GetFullScreenMode()
{
  return(FullScreenHelp);
}


void Help::SetFullScreenMode(int Mode)
{
  FullScreenHelp=Mode;
}


void Help::ReadPluginsHelp()
{
  /* $ 13.07.2000
    ��� �뤥����� ����� �ᯮ�짮������ �㭪�� realloc
  */
  free(HelpData);
  /* SVS $ */
  HelpData=NULL;
  StrCount=0;
  FixCount=1;
  FixSize=2;
  TopStr=0;
  TopicFound=TRUE;
  CurX=CurY=0;
  char PluginsHelpTitle[100];
  sprintf(PluginsHelpTitle,"^ #%s#",MSG(MPluginsHelpTitle));
  AddLine(PluginsHelpTitle);

  for (int I=0;I<CtrlObject->Plugins.PluginsCount;I++)
  {
    char Path[NM],FileName[NM],*Slash;
    strcpy(Path,CtrlObject->Plugins.PluginsData[I].ModuleName);
    if ((Slash=strrchr(Path,'\\'))!=NULL)
      *Slash=0;
    FILE *HelpFile=Language::OpenLangFile(Path,HelpFileMask,Opt.HelpLanguage,FileName);
    if (HelpFile!=NULL)
    {
      char EntryName[512],HelpLine[512],SecondParam[512];
      if (Language::GetLangParam(HelpFile,"PluginContents",EntryName,SecondParam))
      {
        if (*SecondParam)
          sprintf(HelpLine,"   ~%s,%s~@#%s#Contents@",EntryName,SecondParam,Path);
        else
          sprintf(HelpLine,"   ~%s~@#%s#Contents@",EntryName,Path);
        AddLine(HelpLine);
      }
      fclose(HelpFile);
    }
  }
  /* $ 26.06.2000 IS
   ���࠭���� �� � 奫��� �� f1, shift+f2, end (�襭�� �।����� IG)
  */
  AddLine("");
  /* IS $ */
}


int Help::PluginPanelHelp(HANDLE hPlugin)
{
  int PluginNumber=((struct PluginHandle *)hPlugin)->PluginNumber;
  char Path[NM],FileName[NM],StartTopic[256],*Slash;
  strcpy(Path,CtrlObject->Plugins.PluginsData[PluginNumber].ModuleName);
  if ((Slash=strrchr(Path,'\\'))!=NULL)
    *Slash=0;
  FILE *HelpFile=Language::OpenLangFile(Path,HelpFileMask,Opt.HelpLanguage,FileName);
  if (HelpFile==NULL)
    return(FALSE);
  fclose(HelpFile);
  sprintf(StartTopic,"#%s#Contents",Path);
  Help PanelHelp(StartTopic);
  return(TRUE);
}

/* $ 28.06.2000 tran
 (NT Console resize)
 resize help*/
void Help::SetScreenPosition()
{
  if (FullScreenHelp)
    SetPosition(0,0,ScrX,ScrY);
  else
    SetPosition(4,2,ScrX-4,ScrY-2);
  Show();
}
/* tran $ */

