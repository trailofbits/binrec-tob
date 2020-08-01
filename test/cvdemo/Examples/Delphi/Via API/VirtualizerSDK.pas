unit VirtualizerSDK;

{$ALIGN ON}
{$MINENUMSIZE 4}

interface

uses
  Windows;

procedure VirtualizerStart();stdcall;
procedure VirtualizerEnd();stdcall;

{$I VirtualizerSDK_CustomVMsInterface.pas}


implementation

const

{$IFDEF WIN64}
  Virtualizer = 'VirtualizerSDK64.DLL';
{$ELSE}
  Virtualizer = 'VirtualizerSDK32.DLL';
{$ENDIF}

procedure VirtualizerStart; external Virtualizer name 'VirtualizerStart';
procedure VirtualizerEnd; external Virtualizer name 'VirtualizerEnd';

{$I VirtualizerSDK_CustomVMsImplementation.pas}

end.

