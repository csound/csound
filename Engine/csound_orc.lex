Here's how to do a PR:

 *  checkout jpff3develop
 *  create a branch from develop
 *  make your changes, commit
 *  push branch to origin

git push origin branch_name

 *  Open github.com/csound/csound on your browser (you need to be logged
    into github), there will be a banner with a button to create a pull
    request. Follow that, choose develop as the branch against which the PR
    is created.

Or just choose pull requests, create a new pull request using your new
branch as the source, against develop.

should fail but also tell you the exact command to use to push to origin.
For example, I created a branch here called test_branch and when I tried git
push I got:

stevenyi@stevens-MacBook-Pro csound % git checkout -b test_branch
Switched to a new branch 'test_branch'
stevenyi@stevens-MacBook-Pro csound % git push
fatal: The current branch test_branch has no upstream branch.
To push the current branch and set the remote as upstream, use

    git checkout -b $1
    git push
    git push --set-upstream origin $1

To have this happen automatically for branches without a tracking
upstream, see 'push.autoSetupRemote' in 'git help config'.

If I then use git push --set-upstream origin test_branch it should work for
me to push your branch up to Github. (Did not do it as it's just for
testing.)

