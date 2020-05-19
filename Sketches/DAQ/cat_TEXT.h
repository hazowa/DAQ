


void cat_TEXT()
{
  TEXT_parse();
  lcd.setCursor(TEXT_col, TEXT_col);
  lcd.print(TEXT_text);
}
