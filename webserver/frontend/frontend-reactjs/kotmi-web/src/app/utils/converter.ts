
export function convertStringToNumber (stringToConvert: string): number {
    const number = Number(stringToConvert);
    if (Number.isNaN(number)) {
      throw new TypeError('string to number conversion failed');
    }
  
    return number;
  }
  