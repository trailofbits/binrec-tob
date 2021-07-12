object Form1: TForm1
  Left = 418
  Top = 295
  Caption = 'Macros Example'
  ClientHeight = 165
  ClientWidth = 339
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object GroupBox1: TGroupBox
    Left = 40
    Top = 32
    Width = 265
    Height = 105
    Caption = 'Virtualizer Macro'
    TabOrder = 0
    object Button1: TButton
      Left = 8
      Top = 24
      Width = 241
      Height = 25
      Caption = 'Execute Virtualizer Macro #1'
      TabOrder = 0
      OnClick = Button1Click
    end
    object Button2: TButton
      Left = 8
      Top = 56
      Width = 241
      Height = 25
      Caption = 'Execute Virtualizer Macro Level 2'
      TabOrder = 1
      OnClick = Button2Click
    end
  end
end
