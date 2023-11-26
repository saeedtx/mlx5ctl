# Use CentOS as the base image
FROM fedora:34

# Set version as a build argument with a default value
ARG APP_VERSION=1.0

# Set working directory inside the container
WORKDIR /mlx5ctl

# Install necessary build tools and dependencies
RUN dnf install -y rpm-build gcc make

# Copy your C application source code into the container
COPY . .

# Pass the build argument as an environment variable
ENV APP_VERSION=$APP_VERSION

# ... (other Dockerfile commands)

# Use the environment variable in commands if needed
RUN echo "Building version $APP_VERSION"

# CMD or ENTRYPOINT commands if needed

# Set permissions and ownership if needed
# RUN chown -R user:user /app

# Run the commands to build the RPM package
RUN make VERSION=${APP_VERSION} srctar && mkdir -p /root/rpmbuild/SOURCES/ && \
        mv SOURCE/mlx5ctl-${APP_VERSION}.tar.gz /root/rpmbuild/SOURCES/
RUN ls -l /root/rpmbuild/SOURCES/
RUN cd rpm && rpmbuild -bb --define "APP_VERSION ${APP_VERSION}" mlx5ctl.spec --define 'debug_package %{nil}'
RUN mkdir /packages/ && \
        cp /root/rpmbuild/RPMS/x86_64/*.rpm /packages/ && \
        cp /root/rpmbuild/SOURCES/*.tar.gz /packages/

# Define an entry point or any other necessary setup

# CMD or ENTRYPOINT commands if needed
