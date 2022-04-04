FILE=/tmp/upload.tar.bz2
tar caf "$FILE" ".git" $(find . -maxdepth 3 -name "*.pdf") && \
  curl -F "token=$TOKEN" \
       -F "course=$COURSE" \
       -F "module=$MODULE" \
       -F "file=@$FILE" \
       http://jyywiki.cn/upload
