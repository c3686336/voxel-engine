fs = import('fs')

resources = ['right.jpg', 'left.jpg', 'top.jpg', 'bottom.jpg', 'front.jpg', 'back.jpg']

resource_copies = []

foreach resource : resources
  resource_copies += fs.copyfile(resource, resource)
endforeach
