


void cat_TEXT()
{
  TEXT_parse();
  lcd.setCursor(TEXT_col, TEXT_row);
  lcd.print(TEXT_text);
}
