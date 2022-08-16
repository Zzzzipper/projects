export interface IMenu {
  retrospective?: {
    title: string,
    items: { name: string, fileId: string }[]
  },
}
