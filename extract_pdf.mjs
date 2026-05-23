import { createRequire } from 'module';
const require = createRequire(import.meta.url);
const pdfParseModule = require('pdf-parse/node');
import fs from 'fs';

console.log('type:', typeof pdfParseModule);
console.log('keys:', Object.keys(pdfParseModule));

const pdfParse = pdfParseModule.default || pdfParseModule.PdfParse || pdfParseModule;

const buf = fs.readFileSync('C:/FOCP2/darwin-evo.-sim-main/darwin-evo.-sim-main/5C^2 Hackathon.docx.pdf');

if (typeof pdfParse === 'function') {
  const data = await pdfParse(buf);
  console.log(data.text);
} else if (pdfParse && typeof pdfParse.parse === 'function') {
  const data = await pdfParse.parse(buf);
  console.log(data.text);  
} else {
  // Try instantiating
  for (const key of Object.keys(pdfParseModule)) {
    const val = pdfParseModule[key];
    console.log(`  ${key}: ${typeof val}`);
  }
}
