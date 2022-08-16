import { v4 as uuidv4 } from 'uuid';

export function convertStringToNumber (stringToConvert: string): number {
  const number = Number(stringToConvert);
  if (Number.isNaN(number)) {
    throw new TypeError('string to number conversion failed');
  }

  return number;
}

export function convertStringToBoolean (stringToConvert: string): boolean {
  return stringToConvert === 'True';
}

export function uuid () {
  return uuidv4();
}

type NotUndefined<T> = T extends undefined ? never : T;

export function assert (value: any, message: string): asserts value {
  if (!value) {
    throw new TypeError(message);
  }
}

export function assertIsNotUndefined<T> (value: any): asserts value is NotUndefined<T> {
  if (value === undefined) {
    throw new TypeError('not undefined assertion failed');
  }
}
