#ifndef __MANAGER_HPP__
#define __MANAGER_HPP__
/*
manager.hpp

������������ ����� ����������� file panels, viewers, editors

*/

/* Revision: 1.24 21.02.2002 $ */

/*
Modify:
  21.02.2002 SVS
    + ResetLastInputRecord() - ���������� ����� ���������� �������.
  11.10.2001 IS
    + CountFramesWithName
  04.10.2001 OT
    ������ ������������ ������ � ��������� ������
  19.07.2001 OT
    ���������� ����� ����� � ������ ���� Unmodalize��� + ���� ������������
  18.07.2001 OT
    VFMenu
  11.07.2001 OT
    ������� CtrlAltShift � Manager
  26.06.2001 SKV
    + PluginCommit(); (ACTL_COMMIT)
  06.06.2001 OT
    - ���������� �����-�� � ExecuteFrame()...
  28.05.2001 OT
    ! RefreshFrame() �� ��������� ��������� ������� �����
  26.05.2001 OT
    + ����� ������ ExecuteComit(), ExecuteFrame(), IndexOfStack()
    + ����� ���� Frame *ExecutedFrame�
    - ��������� ������ � ����� ���� Destruct
    - �������� ������ ModalSaveState()
  21.05.2001 OT
    + ��������� RefreshedFrame
  21.05.2001 DJ
    ! ������ �������������; � ����� � ���������� ������ ���� �������
      �������� GetFrameTypesCount(); ���������� �������� ������
  16.05.2001 DJ
    ! ����������� ExecuteModal()
  15.05.2001 OT
    ! NWZ -> NFZ
  14.05.2001 OT
    ! ��������� ������� ������ ���������� ReplaceFrame (��� ������������ � ��������)
  12.05.2001 DJ
    ! FrameManager ������� �� CtrlObject, ������� ExecuteModalPtr,
      ReplaceCurrentFrame ������� �� ReplaceFrame, GetCurrentFrame()
  10.05.2001 DJ
    + SwitchToPanels(), ModalStack, ModalSaveState(), ExecuteModalPtr()
  06.05.2001 DJ
    ! �������� #include
    + ReplaceCurrentFrame(), ActivateFrameByPos()
  06.05.2001 ��
    ! �������������� Window � Frame :)
  04.05.2001 DJ
    + ������� � ��������� NWZ
  29.04.2001 ��
    + ��������� NWZ �� ����������
  29.12.2000 IS
    + ����� ExitAll
  28.06.2000 tran
    - NT Console resize bug
      add class member ActiveModal
  25.06.2000 SVS
    ! ���������� Master Copy
    ! ��������� � �������� ���������������� ������
*/

class Frame;

class Manager
{
  private:
    Frame **ModalStack;     // ���� ��������� �������
    int ModalStackCount;    // ������ ����� ��������� �������
    int ModalStackSize;     // ����� ����� ��������� �������

    Frame **FrameList;       // ������� ��������� �������
    int  FrameCount;         // ������ ����������� �������
    int  FrameListSize;      // ������ ������ ��� ����������� �������
    int  FramePos;           // ������ ������� ������������ ������. �� �� ������ ��������� � CurrentFrame
                             // ������� ����������� ����� ����� �������� � ������� FrameManager->GetBottomFrame();

    /*$ ����������� �� ... */
    Frame *InsertedFrame;   // �����, ������� ����� �������� � ����� ����������� �������
    Frame *DeletedFrame;    // �����, �������������� ��� �������� �� ��������� �������, �� ���������� �����, ���� ��������� (�������� ��� �� ���, �� ���)
    Frame *ActivatedFrame;  // �����, ������� ���������� ������������ ����� ����� ������ ���������
    Frame *RefreshedFrame;  // �����, ������� ����� ������ ��������, �.�. ������������
    Frame *ModalizedFrame;  // �����, ������� ���������� � "�������" � �������� ������������ ������
    Frame *UnmodalizedFrame;// �����, ������������ �� "�������" ������������ ������
    Frame *DeactivatedFrame;// �����, ������� ��������� �� ���������� �������� �����
    Frame *ExecutedFrame;   // �����, �������� ��������� ����� ����� ��������� �� ������� ���������� ������

    Frame *CurrentFrame;     // ������� �����. �� ����� ����������� ��� � ����������� �������, ��� � � ��������� �����
                             // ��� ����� �������� � ������� FrameManager->GetCurrentFrame();
    Frame *SemiModalBackFrame;



    int  EndLoop;            // ������� ������ �� �����
    INPUT_RECORD LastInputRecord;
    void StartupMainloop();
    Frame *FrameMenu(); //    ������ void SelectFrame(); // show window menu (F12)

    BOOL Commit();         // ��������� ���������� �� ���������� � ������� � ����� �������
                           // ��� � ����� �������� ����, ���� ������ ���� �� ���������� ������� �� NULL
    // �������, "����������� ����������" - Commit'a
    // ������ ���������� �� ������ �� ���� � �� ������ ����
    void RefreshCommit();  //
    void DeactivateCommit(); //
    void ActivateCommit(); //
    void UpdateCommit();   // ����������� �����, ����� ����� �������� ���� ����� �� ������
    void InsertCommit();
    void DeleteCommit();
    void ExecuteCommit();
    void ModalizeCommit();
    void UnmodalizeCommit();

  public:
    Manager();
    ~Manager();

  public:
    // ��� ������� ����� ��������� �������� ����������� �� ������ ����� ����
    // ��� ��� �� ����������� ���������� � ���, ��� ����� ����� ������� � �������� ��� ��������� ������ Commit()
    void InsertFrame(Frame *NewFrame, int Index=-1);
    void DeleteFrame(Frame *Deleted=NULL);
    void DeleteFrame(int Index);
    void DeactivateFrame (Frame *Deactivated,int Direction);
    void ActivateFrame (Frame *Activated);
    void ActivateFrame (int Index);
    void RefreshFrame(Frame *Refreshed=NULL);
    void RefreshFrame(int Index);

    //! ������� ��� ������� ��������� �������.
    void ExecuteFrame(Frame *Executed);


    //! ������ � ����� ���� ��������� �������
    void ExecuteModal (Frame *Executed=NULL);
    //! ��������� ����������� ����� � ��������� ������
    void ExecuteNonModal();
    //! �������� ����, ��� ����������� ����� ��������� ��� � �� ������� �����.
    BOOL ifDoubleInstance();

    //!  �������, ������� �������� � �������� ���������� ������.
    //  ������ ������������ ������ ��� �������� ��������� � ������� ���������� �������� ���� VFMenu
    void ModalizeFrame (Frame *Modalized=NULL, int Mode=TRUE);
    void UnmodalizeFrame (Frame *Unmodalized);
  private:
    int GetModalExitCode();
    int ModalExitCode;
  public:
    void CloseAll();
    /* $ 29.12.2000 IS
         ������ CloseAll, �� ��������� ����������� ����������� ������ � ����,
         ���� ������������ ��������� ������������� ����.
         ���������� TRUE, ���� ��� ������� � ����� �������� �� ����.
    */
    BOOL ExitAll();
    /* IS $ */
    BOOL IsAnyFrameModified(int Activate);

    int  GetFrameCount() {return(FrameCount);};
    int  GetFrameCountByType(int Type);

    /*$ 26.06.2001 SKV
    ��� ������ ����� ACTL_COMMIT
    */
    BOOL PluginCommit();
    /* SKV$*/

    /* $ 11.10.2001 IS
       ���������� ���������� ������� � ��������� ������.
    */
    int CountFramesWithName(const char *Name, BOOL IgnoreCase=TRUE);
    /* IS $ */

    BOOL IsPanelsActive(); // ������������ ��� ������� WaitInMainLoop
    void SetFramePos(int NewPos);
    int  FindFrameByFile(int ModalType,char *FileName,char *Dir=NULL);
    void ShowBackground();

    void EnterMainLoop();
    void ProcessMainLoop();
    void ExitMainLoop(int Ask);
    int ProcessKey(int key);
    int ProcessMouse(MOUSE_EVENT_RECORD *me);

    void PluginsMenu(); // �������� ���� �� F11
    /* $ 10.05.2001 DJ */
    void SwitchToPanels();
    /* DJ $ */

    INPUT_RECORD *GetLastInputRecord() { return &LastInputRecord; }
    void ResetLastInputRecord() { LastInputRecord.EventType=0; }

    /* $ 12.05.2001 DJ */
    Frame *GetCurrentFrame() { return CurrentFrame; }
    /* DJ $ */

    Frame *operator[](int Index);

    /* $ 19.05.2001 DJ
       operator[] (Frame *) -> IndexOf
    */
    int IndexOf(Frame *Frame);
    /* DJ $ */

    int IndexOfStack(Frame *Frame);
    int HaveAnyFrame();

/* $ ������� ��� ���� CtrlAltShift OT */
    void ImmediateHide();
    Frame *GetBottomFrame() { return (*this)[FramePos]; }
};

extern Manager *FrameManager;

#endif  // __MANAGER_HPP__
