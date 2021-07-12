object Form1: TForm1
  Left = 623
  Top = 668
  Width = 425
  Height = 235
  Caption = 'Macros Example'
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
    Left = 64
    Top = 32
    Width = 297
    Height = 129
    Caption = 'Virtualizer Macro'
    TabOrder = 0
    object Button1: TButton
      Left = 24
      Top = 32
      Width = 249
      Height = 25
      Caption = 'Execute Virtualizer Macro #1'
      TabOrder = 0
      OnClick = Button1Click
    end
    object Button2: TButton
      Left = 24
      Top = 72
      Width = 249
      Height = 25
      Caption = 'Execute Virtualizer Macro #2'
      TabOrder = 1
      OnClick = Button2Click
    end
  end
end
